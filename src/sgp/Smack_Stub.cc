
#if !defined(_MSC_VER)
  #include <strings.h>
#endif
#include <new>
#include <stdexcept>
#include "SDL_pixels.h"
#include "SDL_timer.h"

#include "Debug.h"
#include "Smack_Stub.h"
#include "Sound_Control.h"
#include "SoundMan.h"
#include "FileMan.h"
#include "Video.h"
#include "VSurface.h"

extern "C" {
#include "smacker.h"
}

#define SMKTRACK 0

struct Smack
{
  smk Smacker; //object pointer type for libsmacker
  SDL_Surface *Surface;
  void *OldPixels; // the original value of Surface->pixels
  UINT32 SoundTag; // for soundman
  unsigned long Height;
  unsigned long Width;
  unsigned long Frames;
  UINT32 MicrosecondsPerFrame;
  UINT32 LastTick;
};


static void SmackCheckStatus(INT8 smkstatus) {
  if (smkstatus <0) {
    throw std::runtime_error("SmackLibrary returned an error");
  }
}


// read all smackaudio and convert it to 22050Hz on the fly (44100 originally)
static UINT32 SmackGetAudio (const smk SmkObj, INT16* audiobuffer) 
{
  UINT32 n_samples = 0,  smacklen = 0;
  UINT32 i, index;
  INT16 *smackaudio, *paudio = audiobuffer;
  if (! audiobuffer ) return 0;

  smk_enable_audio(SmkObj, SMKTRACK, true);
  smk_first(SmkObj);
  do {
    smacklen = smk_get_audio_size (SmkObj, SMKTRACK);
    smackaudio = (INT16*) smk_get_audio (SmkObj, SMKTRACK);
    index = 0; 
    for  ( i = 0 ; i < smacklen/8; i++ )
      {
        *paudio++ = ((smackaudio[ index   ] +smackaudio[index+2]) / 2 );
        *paudio++ = ((smackaudio[ index +1] +smackaudio[index+3]) / 2 );
        index += 4;
      } 
    n_samples += i;
  } while (  smk_next(SmkObj) != SMK_DONE  );

  smk_enable_audio(SmkObj, SMKTRACK, false);
  return n_samples;
}


Smack* SmackOpen(SGPFile* FileHandle)
{
  Smack* flickinfo = NULL;
  INT16* audiobuffer = NULL;

try {
  flickinfo = new Smack();
  memset(flickinfo, 0, sizeof(Smack));

  UINT32 smacksize = FileGetSize(FileHandle);
  uint8_t *smacktomemory = new uint8_t[smacksize];
  FileRead (FileHandle, smacktomemory, smacksize);
  flickinfo->Smacker = smk_open_memory(smacktomemory, smacksize);
  // Quoting http://libsmacker.sourceforge.net , smk_open_memory:
  // "You may free the buffer after initialization."
  delete [] smacktomemory;

  if (! flickinfo->Smacker ) throw std::runtime_error("smk_open_memory failed");

  INT8 smkstatus;
  smkstatus = smk_info_video(flickinfo->Smacker, &flickinfo->Width, &flickinfo->Height, NULL);
  SmackCheckStatus(smkstatus);

  DOUBLE usf;
  smkstatus = smk_info_all(flickinfo->Smacker, NULL, &flickinfo->Frames, &usf);
  SmackCheckStatus(smkstatus);
  flickinfo->MicrosecondsPerFrame = usf;

  UINT8    a_depth[7], a_channels[7];
  /* arrays for audio track metadata */
  unsigned long   a_rate[7];
  smkstatus = smk_info_audio(flickinfo->Smacker,  NULL, a_channels, a_depth, a_rate);
  SmackCheckStatus(smkstatus);

  flickinfo->Surface = SDL_CreateRGBSurfaceWithFormat(0,
      flickinfo->Width, flickinfo->Height, 24, SDL_PIXELFORMAT_INDEX8);
  if (!flickinfo->Surface) throw std::runtime_error("SDL_CreateRGBSurfaceWithFormat failed");
  flickinfo->OldPixels = flickinfo->Surface->pixels;

  // calculated audio memory for downsampling 44100->22050
  unsigned long audiolen, audiosamples;
  audiosamples = ( (flickinfo->Frames / (usf/1000)) * (a_rate[SMKTRACK]/2) * 16 *  a_channels[SMKTRACK]);
  audiobuffer = (INT16*)malloc( audiosamples );
  if ( ! audiobuffer ) throw std::bad_alloc();

  smk_enable_video(flickinfo->Smacker, false);
  audiolen = SmackGetAudio (flickinfo->Smacker, audiobuffer);
  smk_enable_video(flickinfo->Smacker, true);

  // shoot and forget... audiobuffer should be freed by SoundMan
  if (audiolen > 0) {
    flickinfo->SoundTag = SoundPlayFromBuffer(audiobuffer, audiolen, MAXVOLUME, 64, 1, NULL, NULL);
  } else {
    // Ok, go on with just video, no sound
    free(audiobuffer);
  }
  audiobuffer = NULL; // make sure the exception handler does not free this buffer

  smkstatus = smk_first(flickinfo->Smacker);
  SmackCheckStatus(smkstatus);

  flickinfo->LastTick = SDL_GetTicks();
  return flickinfo;
}
catch (...) {
  if (flickinfo) SmackClose(flickinfo);
  free(audiobuffer);
  throw;
}
}

void BlitCurrentFrame(Smack* Smk, UINT32 Left, UINT32 Top);
void SmackDoFrame(Smack* Smk, UINT32 Left, UINT32 Top)
{
  BlitCurrentFrame(Smk, Left, Top);

  UINT32 i=0;
  // wait for FPS milliseconds
  UINT16 millisecondspassed = SDL_GetTicks() - Smk->LastTick;
  UINT16 skiptime;
  UINT16 delay, skipframes = 0;
  DOUBLE framerate = Smk->MicrosecondsPerFrame/1000;

  if (  framerate > millisecondspassed ) {
    delay = framerate-millisecondspassed;
  } 
  else // video is delayed - so skip frames according to delay but take fps into account
    {
      skipframes = millisecondspassed / (UINT16)framerate;
      delay = millisecondspassed % (UINT16)framerate;
      //bigskiptime:
      skiptime = SDL_GetTicks();
      millisecondspassed = skiptime - Smk->LastTick;
       while (skipframes > 0) {
        SmackNextFrame(Smk);
        skipframes--;
        i++;
      }
      skiptime = SDL_GetTicks() - skiptime;
      if (skiptime+delay <= i*framerate) {
        delay =  i*(framerate)-skiptime-delay ;
      }
      else { delay =  0; 
      // need to find a nice way to compensate for lagging video
        //Smk->LastTick = SDL_GetTicks(); 
        //skipframes = skiptime+delay / (UINT16)framerate;
        //delay = (skiptime+delay) % (UINT16)framerate;
        //i=0;
        //goto bigskiptime; // skiptime was big.. so just go on skipping frames
      }
    }
  SDL_Delay(delay);
  Smk->LastTick = SDL_GetTicks();
}


INT8 SmackNextFrame(Smack* Smk)
{
  return smk_next(Smk->Smacker);
}


void SmackClose(Smack* Smk)
{
  if (Smk->SoundTag) SoundStop(Smk->SoundTag);
  if (Smk->Smacker) smk_close (Smk->Smacker); // closes and frees Smacker Object and file
  if (Smk->Surface) {
    Smk->Surface->pixels = Smk->OldPixels;
    SDL_FreeSurface(Smk->Surface);
  }
  delete Smk;
}


void BlitCurrentFrame(Smack* Smk, UINT32 const Left, UINT32 const Top)
{
  uint8_t const *smk_palette = smk_get_palette(Smk->Smacker);
  SDL_Color sdl_palette[256];
  for (int i = 0; i < 256; ++i) {
    sdl_palette[i].r = smk_palette[3 * i + 0];
    sdl_palette[i].g = smk_palette[3 * i + 1];
    sdl_palette[i].b = smk_palette[3 * i + 2];
    sdl_palette[i].a = 255;
  }

  SDL_SetPaletteColors(Smk->Surface->format->palette, sdl_palette, 0, 256);
  Smk->Surface->pixels = smk_get_video(Smk->Smacker);

  SDL_Rect dstRect = { int(Left), int(Top), int(Smk->Width), int(Smk->Height) };
  SDL_BlitSurface(Smk->Surface, NULL, FRAME_BUFFER->surface_, &dstRect);
}
