#pragma once

void displayBegin();       // init LCD + splash (blocking 1x saat setup)
void displayUpdate();      // non-blocking, hanya menulis karakter yang berubah
void displayShowAbout();   // tampilkan layar ABOUT selama 10 detik
