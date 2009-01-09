/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#include "nui.h"
#include "nuiFileSelector.h"
#include "nuiFileTree.h"


#ifdef MYDEBUG


//**************************************************************************************************************
//
// nuiFileTree
//
//**************************************************************************************************************

nuiFileTree::nuiFileTree(const nglPath& rPath, const nglPath& rRootPath, const nglString& rFilter, bool showHiddenFiles)
: nuiFileSelectorBase(), mEventSink(this)
{
  std::list<nglString> filters;
  if (rFilter != nglString::Null)
    filters.push_back(rFilter);
  Init(rPath, rRootPath, filters, showHiddenFiles);
}

nuiFileTree::nuiFileTree(const nglPath& rPath, const nglPath& rRootPath, const std::list<nglString>& rFilters, bool showHiddenFiles)
: nuiFileSelectorBase(), mEventSink(this)
{
  Init(rPath, rRootPath, rFilters, showHiddenFiles);  
}

void nuiFileTree::Init(const nglPath& rPath, const nglPath& rRootPath, const std::list<nglString>& rFilters, bool showHiddenFiles)
{
  SetObjectClass(_T("nuiFileTree"));
  SetObjectName(_T("nuiFileTree"));
  
  mpTreeView = NULL;
  mShowHiddenFiles = showHiddenFiles;
  mFilters = rFilters;
  mpCurrentTree = NULL;
  
  SetRootPath(rRootPath);
  SetPath(rPath);  
}


nuiFileTree::~nuiFileTree()
{
}



//******************************************************************

void nuiFileTree::FormatFileSize(nuiSize size, nglString& str)
{
  if (size > 1000000000)
    str.Format(_T("%.2f Go"), size / (1024*1024*1024));
  else if (size > 1000000)
    str.Format(_T("%.2f Mo"), size / (1024*1024));
  else if (size > 1000)
    str.Format(_T("%.2f Ko"), size / 1024);
  else 
    str.Format(_T("%d b"), size);  
}


nuiWidget* nuiFileTree::GetFileInfo(const nglPath& rPath)
{
  nuiSimpleContainer* pMainCont = new nuiSimpleContainer();
  pMainCont->SetObjectName(_T("nuiFileTree::InfoView"));
  pMainCont->SetObjectClass(_T("nuiFileTree::InfoView"));
  
  nuiVBox* pBox = new nuiVBox(0);
  pBox->SetExpand(nuiExpandShrinkAndGrow);
  pMainCont->AddChild(pBox);
  
  // file icon
  nuiSimpleContainer* pIcon = new nuiSimpleContainer();
  pBox->AddCell(pIcon, nuiCenter);
  
  nglString objectName;
  objectName.Format(_T("nuiFileTree::InfoViewIcon::%ls"), rPath.GetExtension().GetChars());
  pIcon->SetObjectName(objectName);
  pIcon->SetObjectClass(objectName);
  
  // file name
  nuiLabel* pLabel = new nuiLabel(rPath.GetNodeName());
  pLabel->UseEllipsis(true);
  pBox->AddCell(pLabel, nuiCenter);
  
  // file size
  nglPathInfo info;
  rPath.GetInfo(info);
  nglString sizestr;
  FormatFileSize(info.Size, sizestr);
  pBox->AddCell(new nuiLabel(sizestr), nuiCenter);
  
  // file modification date
  nglTimeInfo timeInfo;
  nglString timestr;
  nglTime lastMod = info.LastMod;
  lastMod.GetLocalTime(timeInfo);
  timestr.Format(_T("%d / %d / %d   %d:%d"), timeInfo.Year+1900, timeInfo.Month, timeInfo.Day, timeInfo.Hours, timeInfo.Minutes);
  pBox->AddCell(new nuiLabel(timestr), nuiCenter);

  
  return pMainCont;
}






bool nuiFileTree::SetRootPath(const nglPath& rPath)
{
  mRootPath = rPath;
  
  mEventSink.DisconnectAll();
  mTrees.clear();
  
  if (mpTreeView)
    mpTreeView->Trash();
  
  mpTreeRoot = new nuiTreeNode(new nuiLabel(_T("TreeRoot")));
  mpTreeView = new nuiTreeView(mpTreeRoot, false);
  
  AddChild(mpTreeView);
  
  
  if (rPath.GetPathName().Compare(ROOTPATH_ALLVOLUMES))
  {
    AddTree(rPath, true);
  }
  
  else
  {
    std::list<nglPathVolume> volumes;
    std::list<nglPathVolume>::iterator it;
    
    nglPath::GetVolumes(volumes, nglPathVolume::All);
    for (it = volumes.begin(); it != volumes.end(); ++it)
    {
      nglPathVolume volume = *it;
      
      AddTree(volume.mPath, true);    
    }
  }
  
  
  return true;
}



void nuiFileTree::AddTree(const nglPath& rPath, bool Opened)
{
  nuiTreeNodePtr pNode = GetNewNode(rPath);
  pNode->Open(Opened);
  
  mpTreeRoot->AddChild(pNode);
  
  mTrees[rPath] = pNode;  
}




bool nuiFileTree::SetPath(const nglPath& rPath)
{
  //LBDEBUG
  NGL_OUT(_T("\nSetPath ENTRY\n"));
  ///////////
  
  nglPath path(rPath);
  
  nuiTreeNode* pNode = NULL;
  nglPath rootPath;
  
  // find which rootpath for this Path
  nglPath tmpPath(rPath);
  while (!pNode)
  {
    std::map<nglPath,nuiTreeNode*>::iterator it;
    for (it = mTrees.begin(); it != mTrees.end(); ++it)
    {
      const nglPath rRootPath = it->first;
      
      
      //LBDEBUG
      NGL_OUT(_T("SetPath rootpath '%ls'  compared to '%ls'\n"), rRootPath.GetChars(), tmpPath.GetChars());
      ///////////
      
      if (rRootPath == tmpPath)
      {
        rootPath = rRootPath;
        pNode = it->second;

        //LBDEBUG
        NGL_OUT(_T("SetPath OK rootpath accepted with '%ls' : pNode '%ls'\n"), tmpPath.GetChars(), pNode->GetProperty(_T("Path")).GetChars());
        ///////////
        
        break;
      }
    }
          
    tmpPath = tmpPath.GetParent();
    if (tmpPath.GetPathName().IsEmpty() && !pNode)
      break;
  }
  
  if (!pNode)
  {
    //LBDEBUG
    NGL_OUT(_T("SetPath ERROR not rootpath found!\n\n"), rootPath.GetChars());
    ///////////
    
    return false;
  }
  
  
  path.MakeRelativeTo(rootPath);
  
  //LBDEBUG
  NGL_OUT(_T("path has been made relartive : '%ls'\n"), path.GetChars());
  ///////////
  
  std::vector<nglString> tokens;
  path.GetPathName().Tokenize(tokens, '/');
  
  pNode->Select(false);
  
  for (uint i = 0; i < tokens.size(); i++)
  {
    pNode->OpenAllChildren(false);
    pNode->SelectAllChildren(false);
    
    std::vector<nuiTreePtr> children;
    pNode->GetChildren(children);
    for (uint j = 0; j< children.size(); j++)
    {
      nuiFileSelectorNode* pBNode = dynamic_cast<nuiFileSelectorNode*>(children.at(j));
      if (pBNode)
      {
        nglPath p(pBNode->GetProperty(_T("Path")));
        
        //LBDEBUG
        NGL_OUT(_T("p.GetNodeName() '%ls' compare to tokens(%d) '%ls'\n"), p.GetNodeName().GetChars(), i, tokens.at(i).GetChars());
        ////////////
        
        if (p.GetNodeName() == tokens.at(i))
        {
          //LBDEBUG
          NGL_OUT(_T("OK\n"));
          ////////////

          pNode = pBNode;
          break;
        }
      }
      
    }
    
    if (pNode == mpTreeView->GetSelectedNode()) // Have we found the next node?
      return false; // No next node found: bail out...
        
    nuiTreeNodePtr pParent = pNode;
    while (pParent)
    {
      if (!pParent->IsEmpty())
        pParent->Open(true);
      pParent = static_cast<nuiTreeNodePtr>(pParent->GetParent());
    }

    if (pNode->IsEmpty()) // Bail out if the last node is a leaf
    {
      PathChanged();
      mpTreeView->SelectionChanged();
      mpTreeView->SetRect(mpTreeView->GetIdealRect());
      return false;
    }
    
  }
  
  
  PathChanged();
  mpTreeView->SelectionChanged();
  mpTreeView->SetRect(mpTreeView->GetIdealRect());
  
  
  // Update infos and change the edit line:
  nglPathInfo info;
  rPath.GetInfo(info);

  mpTreeView->SetHotRect(mpTreeView->GetHotRect());

  return true;
}




nglPath nuiFileTree::GetPath() const
{
  nuiTreeNode* pTree = mpTreeView->GetSelectedNode();
  if (!pTree)
    return nglPath(nglString::Null);

	nglPath pathName = pTree->GetProperty(_T("Path"));
	nglString node = pathName.GetNodeName();
	
	nglString addin;
	
	if ((addin != nglString::Null) && (node.Compare(addin)))
	{
		if (pathName.IsLeaf())
			return pathName.GetParent() + addin;
			
		return pathName + addin;
	}
	
	return pathName;
}




nglPath nuiFileTree::GetRootPath() const
{
  return mRootPath;
}






//bool nuiFileTree::OnRootOpened(const nuiEvent& rEvent)
//{
//  nuiTreeView* pTreeView = (nuiTreeView*)rEvent.mpUser;
//  
//  std::map<nglPath, nuiTreeView*>::iterator it;
//  for (it = mTreeViews.begin(); it != mTreeViews.end(); ++it)
//  {
//    nuiTreeView* pPtr = it->second;
//    
//    if (pPtr != pTreeView)
//      pTreeView->GetTree()->Open(false);
//  }
//  return true;
//}





bool nuiFileTree::OnSelected (const nuiEvent& rEvent)
{
  nuiTreeView* pTreeView = (nuiTreeView*)rEvent.mpUser;
  NGL_ASSERT(pTreeView);
  
//	nglPath curPath;
//  nuiTreeNode* pTree = pTreeView->GetSelectedNode();
//	if (pTree != NULL)
//		curPath = pTree->GetProperty(_T("Path"));
//  
//   
//	if (curPath.IsLeaf())
//  {
//		nglString addin = curPath.GetNodeName();
//  }
//	
	return false;
}
  


#endif


