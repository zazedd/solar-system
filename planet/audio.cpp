#include <audio.h>
#define MINIAUDIO_IMPLEMENTATION
#include <soloud.h>
#include <soloud_wav.h>
#include <stdio.h>

using namespace std;
void initializeMiniaudio() {

  SoLoud::Soloud gSoloud; // SoLoud engine
  SoLoud::Wav gWave;

  gSoloud.init(); // Initialize SoLoud

  srand((unsigned int)time(NULL));

  int order[3] = {0, 1, 2};
  for (int i = 2; i > 0; i--) {
    int j = rand() % (i + 1);
    int temp = order[i];
    order[i] = order[j];
    order[j] = temp;
  }

  if (gWave.load(songs[order[0]]) == -1) {
    cout << "Error while loading song" << endl;
    exit(-1);
  };

  int current = 0;
  float len = gWave.getLength();
  float t = 0.0f;
  float lastT = 0.0f;

  gSoloud.play(gWave);

  while (!glfwWindowShouldClose(window)) {
    gSoloud.setGlobalVolume(volume);

    t = glfwGetTime() - lastT;

    if (t > len || shouldSkip) {
      gWave.stop();
      current++;
      if (current > 2) {
        cout << "No more songs on the playlist" << endl;
        break;
      }

      if (gWave.load(songs[order[current]]) == -1) {
        cout << "Error while loading song" << endl;
        exit(-1);
      };

      len = gWave.getLength();
      lastT = t;
      t = 0.0f;
      gSoloud.play(gWave);

      shouldSkip = false;
    }
  }

  gSoloud.deinit();
  return;
}
