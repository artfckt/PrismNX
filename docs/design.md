# SwitchColor pentru Switch Lite

Tinta utilizatorului: Switch Lite, Atmosphere 1.11.2|S, firmware raportat 20.5.0.
Versiunile sunt pastrate asa cum au fost comunicate; compatibilitatea pe consola
va trebui verificata pe dispozitiv, nu dedusa doar din compilare.

## Arhitectura aleasa

Overlay nativ C++ `.ovl` pentru Tesla/Ultrahand, construit cu devkitA64/libnx.
Client IPC pentru serviciul Fizeau; Fizeau gestioneaza unitatea hardware CMU.
Folosim ecranul intern, potrivit pentru Switch Lite. Nu accesam memoria jocului.

Alternative evaluate: modificarea intregii aplicatii Fizeau mareste suprafata
de intretinere; un motor propriu de procesare necesita cercetare firmware/GPU.
Un overlay propriu peste serviciul existent permite o prima versiune utilizabila.

## Prima versiune

- Reglaje live: saturatie, contrast, gamma, temperatura, nuanta si luminanta.
- Standard, Vibrant, Cinema si Night sunt preseturi manuale, nu calibrari de ecran.
- Activare/dezactivare, resetare neutra si restaurarea starii de la deschidere.
- Salvare explicita pentru repornire in configuratia Fizeau, cu copie de rezerva.
- Citirea si pastrarea celorlalte profiluri; modificarile vizeaza profilul intern.
- Ziua si noaptea primesc aceleasi reglaje pentru un rezultat manual constant.
- Erorile IPC si de card SD sunt afisate; un serviciu absent nu blocheaza overlay-ul.

Sharpness spatial nu este oferit de CMU. Prima versiune nu prezinta un slider
nefunctional; include o explicatie scurta in pagina de informatii.

## Verificare si livrare

Compilare ARM64 reala, verificarea formatului NRO/OVL, teste pentru limite,
persistenta si tranzactii IPC folosind un serviciu simulat pe calculator.
Arhiva de instalare a overlay-ului, surse si ghid in romana. Fizeau ramane o
dependenta separata pentru a evita suprascrierea unei instalari existente.
Testele pe PC nu certifica functionarea pe consola; ghidul include verificarile
in joc, la suspendare/revenire si dupa repornire.

## Surse verificate

- https://github.com/averne/Fizeau/tree/v2.8.3
- https://github.com/WerWolv/libtesla
- https://github.com/ppkantorski/Ultrahand-Overlay
- https://github.com/Atmosphere-NX/Atmosphere/releases/tag/1.11.2
