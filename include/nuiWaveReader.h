/*
 NUI3 - C++ cross-platform GUI framework for OpenGL based applications
 Copyright (C) 2002-2003 Sebastien Metrot
 
 licence: see nui3/LICENCE.TXT
 */

#pragma once

 #include "nui.h"
 #include "nuiChunkSampleReader.h"

class nuiWaveReader : public nuiChunkSampleReader
{
public:
  nuiWaveReader(nglIStream& rStream); ///< nuiWaveReader Constructor \param rStream The nglIStream in which we read
  nuiWaveReader(const nuiWaveReader& rReader, nglIStream& rStream);
  virtual ~nuiWaveReader();
  
  virtual nuiSampleReader* Clone(nglIStream& rStream) const;

  uint32 ReadIN(void* pBuffer, uint32 sampleframes, nuiSampleBitFormat format = eSampleFloat32);
  
  virtual void SetPosition(uint32 position);
private:
  virtual bool ReadInfo();
};
