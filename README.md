# SwitchColor

Toolkit overlay nativ pentru **Nintendo Switch Lite modat**: imagine,
informatii live, control rapid si acces la unelte, prin Tesla/Ultrahand.
Reglajele de culoare folosesc serviciul Fizeau. Tinta comunicata: Atmosphere 1.11.2|S,
firmware 20.5.0. Versiunea firmware este pastrata exact cum a fost raportata.

**Stare:** versiunea 0.2.0, compilata ARM64 si testata pe calculator.
Nu a fost testata pe consola. Compatibilitatea efectiva cu configuratia de mai
sus trebuie verificata pe Switch; compilarea nu o certifica.

## Ce include

- Saturatie, contrast, gamma, temperatura culorilor, nuanta si luminanta.
- Slidere cu D-pad si atingere; aplicare live cu limitarea frecventei IPC.
- 18 preseturi in patru categorii: stil OLED, contrast, cald si creativ.
- Meniu central cu sapte pagini de informatii reale despre consola.
- Baterie, temperaturi, frecvente CPU/GPU/EMC, retea, firmware si stocare SD.
- Luminozitate fizica si volum, cu verificarea valorii dupa modificare.
- Export diagnostic local si scurtaturi catre patru overlay-uri instalate.
- Activare/dezactivare, resetare neutra, restaurarea starii de la deschidere.
- Salvare explicita pentru repornire, cu pastrarea primei copii de rezerva.
- Erori vizibile si recuperare explicita dupa un rezultat IPC incert.

**Sharpness spatial nu este implementat.** Fizeau modifica fiecare pixel prin
matrice de culoare si LUT; accentuarea contururilor necesita acces la pixelii
vecini. Luminanta ajusteaza tonurile imaginii, nu lumina de fundal a ecranului.

Setarile sunt globale pentru ecranul intern, nu profiluri automate per joc.
Prima modificare a unui slider foloseste valorile de zi ca punct de plecare si
le aplica atat ziua, cat si noaptea. Sliderele pastreaza programul, dimming-ul,
canalele si filtrul existent. Preseturile/resetarea activeaza toate canalele,
elimina filtrul monocrom, restabilesc intervalul RGB complet si opresc dimming-ul
Fizeau. Starea Pornita/Oprita ramane la alegerea utilizatorului.

Vezi [ghidul toolkit si directiile urmatoare](docs/TOOLKIT_RO.md).

## Instalare

Citeste [ghidul de instalare](docs/INSTALL_RO.md). Pachetul complet este generat
in `dist/SwitchColor-0.2.0-SwitchLite.zip`; contine `sd/` pentru fisierele de pe
card si `optional/config-initiala.ini` doar pentru o instalare Fizeau noua.

Nu include Tesla/Ultrahand sau nx-ovlloader. Foloseste instalarea compatibila
de pe consola. Overlay-ul nu inlocuieste meniul acestora.

Backend-ul distribuit este **Fizeau 2.8.3 + o corectie locala la citirea ultimului
profil din configuratie**. Nu este o versiune oficiala Fizeau noua. Corectia
este in `patches/fizeau-config-eof.patch`; codul upstream ramane nemodificat
in submodul. Sunt incluse patch-urile CMU din arhiva oficiala verificata SHA256.

## Compilare pe Windows

Necesita devkitPro, devkitA64, libnx, switch-glm, Python 3 si MSYS2.
Testele pe calculator folosesc pachetul MSYS2 `gcc`.

```powershell
git submodule update --init third_party/fizeau
git -C third_party/fizeau submodule update --init lib/libtesla lib/inih/inih
.\scripts\build.ps1 -DevkitPro 'E:\Code\devkitPro'
.\scripts\build.ps1 -DevkitPro 'E:\Code\devkitPro' -Test
.\scripts\build-backend.ps1 -DevkitPro 'E:\Code\devkitPro'
python tests/test_backend_boot.py --bash 'E:\Code\devkitPro\msys2\usr\bin\bash.exe'
python scripts/package.py
```

Pentru arhiva de surse extrasa, omite cele doua comenzi `git submodule`:
dependentele folosite sunt deja incluse.

In MSYS2, pachetele suplimentare se instaleaza prin
`pacman -S --needed gcc switch-glm`. `scripts/package.py` descarca numai arhiva
oficiala Fizeau v2.8.3, verifica hash-ul fixat si extrage patch-urile CMU.

Pentru Linux: `make`, `make test`, `python3 scripts/prepare_backend.py`, apoi
`make -C build/fizeau-backend/sysmodule` intr-un mediu devkitPro. Python trebuie
sa fie accesibil si in shell-ul care executa `make`.

Structura: `main.cpp` este meniul central; `*_ui.cpp` contin paginile;
`ui.cpp` contine starea si elementele comune; `telemetry.cpp` citeste serviciile
si executa comenzile rapide; `presets.cpp` defineste catalogul. `model.cpp`
defineste reglajele; `backend.cpp` verifica tranzactiile; `switch_backend.cpp`
foloseste IPC real; `storage.cpp` scrie configuratia. Sursele complete folosite pentru pachet sunt
incluse separat in arhiva `SwitchColor-0.2.0-source.zip`.

## Verificare

Testele executa codul de productie pentru limite numerice, NaN/Inf, preseturi,
structuri IPC, serviciu absent, rollback, stare incerta, conservarea profilurilor,
precedenta fisierelor, backup, scrieri incomplete si erori de redenumire.
Testul de pornire foloseste parserul real Fizeau/inih si demonstreaza regresia
ultimului profil si corectia. Nu simuleaza driverul video sau hardware-ul CMU.

Vezi [licentele si dependentele](THIRD_PARTY.md). Codul SwitchColor este
GPL-2.0-or-later.
