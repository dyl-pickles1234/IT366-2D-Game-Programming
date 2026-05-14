#include "simple_logger.h"

#include "audio.h"

GFC_List* get_beats(GFC_Sound* music) {
  if (!music) return NULL;

  // slog("%s has %i samples", music->filepath, music->sound->alen / 2 / 2); // 16-bit & stereo

  float lenvelope = 0.0f;
  float renvelope = 0.0f;
  float smoothing = 0.001f;
  int cutoff = 500;
  float threshold_u = 500;
  float threshold_l = 300;

  Uint8 linPulse = false;
  Uint8 rinPulse = false;

  GFC_List* beats = gfc_list_new();

  short prevlvalue = (Uint16)music->sound->abuf[0] | ((Uint16)music->sound->abuf[1] << 8);
  short prevrvalue = (Uint16)music->sound->abuf[2] | ((Uint16)music->sound->abuf[3] << 8);
  for (int i = 0; i < music->sound->alen; i += 4) {
    short lvalue = (Uint16)music->sound->abuf[i] | ((Uint16)music->sound->abuf[i + 1] << 8);
    short rvalue = (Uint16)music->sound->abuf[i + 2] | ((Uint16)music->sound->abuf[i + 3] << 8);
    // printf(" %i %i ", lvalue, rvalue);

    // low pass
    short newlvalue = smoothing * lvalue + (1.0f - smoothing) * prevlvalue;
    short newrvalue = smoothing * rvalue + (1.0f - smoothing) * prevrvalue;

    prevlvalue = newlvalue;
    prevrvalue = newrvalue;

    // remove anything too quiet (olnly keep low frequency blobs)
    if (abs(newlvalue) < cutoff) newlvalue = 0;
    if (abs(newrvalue) < cutoff) newrvalue = 0;

    newlvalue = abs(newlvalue);
    newrvalue = abs(newrvalue);

    lenvelope = lenvelope * (1.0f - smoothing) + newlvalue * smoothing;
    renvelope = renvelope * (1.0f - smoothing) + newrvalue * smoothing;

    if (!linPulse && lenvelope > threshold_u) {
      linPulse = true;
      gfc_list_append(beats, (void*)(i / 4));
      // printf("Pulse:%i-Sample:%i ", i / 4, newlvalue);
    }
    if (linPulse && lenvelope < threshold_l) {
      linPulse = false;
    }

    if (!rinPulse && lenvelope > threshold_u) {
      rinPulse = true;
      // printf("Pulse started");
    }
    if (rinPulse && lenvelope < threshold_l) {
      rinPulse = false;
    }

    // music->sound->abuf[i] = newlvalue & 0x00FF;
    // music->sound->abuf[i + 1] = newlvalue >> 8 & 0x00FF;
    // music->sound->abuf[i + 2] = newrvalue & 0x00FF;
    // music->sound->abuf[i + 3] = newrvalue >> 8 & 0x00FF;

    // printf(" %i %i ", newlvalue, newrvalue);
  }

  return beats;
}