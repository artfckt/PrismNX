# Instalare SwitchColor 0.1.0 pe Switch Lite

Pachetul este compilat, dar inca netestat pe consola. Tinta indicata este Switch
Lite / Atmosphere 1.11.2|S / firmware raportat 20.5.0. Nu este necesara o schimbare
de firmware pentru a copia pachetul; functionarea trebuie verificata local.

## 1. Pregateste meniul overlay

Trebuie sa ai Tesla sau Ultrahand si nx-ovlloader instalate si functionale.
Deschide meniul prin combinatia configurata pe consola. Combinatia poate varia;
SwitchColor nu o modifica. Daca nu ai un meniu:

- https://github.com/ppkantorski/Ultrahand-Overlay
- https://github.com/WerWolv/Tesla-Menu
- https://github.com/WerWolv/nx-ovlloader

## 2. Copiaza fisierele

1. Opreste consola si deschide cardul SD pe calculator.
2. Daca ai Fizeau, pastreaza o copie a folderului
   `atmosphere/contents/0100000000000F12` si a configuratiei existente.
3. Extrage `SwitchColor-0.1.0-SwitchLite.zip` pe calculator.
4. Copiaza **continutul folderului `sd`** in radacina cardului SD, combinand
   directoarele. Nu copia folderul `sd` ca atare.

Fisiere principale:

```text
switch/.overlays/SwitchColor.ovl
atmosphere/contents/0100000000000F12/exefs.nsp
atmosphere/contents/0100000000000F12/flags/boot2.flag
atmosphere/exefs_patches/nvnflinger_cmu/*.ips
```

Modulul Fizeau de la acelasi ID este inlocuit cu build-ul 2.8.3 corectat local.
Nu instala un al doilea modul Fizeau cu alt ID. Meniul Tesla/Ultrahand ramane
cel existent. Arhiva nu contine `ovlmenu.ovl` sau configuratia ta Fizeau.

## 3. Doar daca nu ai o configuratie Fizeau

Verifica ambele locatii, in aceasta ordine:

```text
switch/Fizeau/config.ini
config/Fizeau/config.ini
```

Daca exista oricare dintre ele, **pastreaz-o**. Prima locatie are prioritate.
Daca nu exista niciuna, copiaza `optional/config-initiala.ini` din pachet ca
`config/Fizeau/config.ini` pe card. Creeaza directoarele daca lipsesc.
Configuratia initiala este neutra si are patru profiluri complete.

## 4. Folosire

1. Reporneste consola in Atmosphere.
2. Porneste un joc, deschide Tesla/Ultrahand si selecteaza **SwitchColor**.
3. Lasa „Corectie imagine” pornita; selecteaza un reglaj cu A si modifica-l
   cu stanga/dreapta sau atingere. B revine la meniul precedent.
4. Foloseste Pornita/Oprita pentru comparatia imaginii. „Resetare neutra”
   aplica valorile standard Fizeau. „Restabileste starea initiala” readuce
   profilul si activarea de la deschiderea overlay-ului.
5. „Salveaza pentru repornire” scrie explicit configuratia pe SD.

Modificarile live continua dupa inchiderea overlay-ului. Fara salvare, la
repornire revin valorile din fisier. Restaurarea starii initiale este **doar
live**: dupa o salvare, salveaza din nou daca vrei sa pastrezi starea restaurata.

Salvarea creeaza prima copie `config.ini.switchcolor.bak` si o pastreaza la
salvarile ulterioare. Configuratiile structurale invalide sunt refuzate.
Comentariile si cheile necunoscute din profiluri sunt pastrate. Cheile globale
necunoscute sunt refuzate, fiind incompatibile cu parserul Fizeau.

## Erori si revenire

- **Serviciul Fizeau nu ruleaza:** verifica modulul, `boot2.flag` si repornirea
  in Atmosphere. Apoi foloseste „Reincearca”.
- **Configuratie invalida:** verifica existenta unui fisier complet si valid.
  Fa o copie a fisierului actual inainte de a incerca sablonul neutru.
- **Stare incerta:** nu salva. Alege „Recuperare dupa eroare”. Daca recuperarea
  esueaza, reporneste consola; nu este suficienta o simpla citire a valorilor.
- **Eroare SD:** spatiul liber, permisiunile si fisierele temporare sunt
  mentionate in detalii. Fisierele `.switchcolor.tmp`, `.switchcolor.rollback`
  sau `.switchcolor.bak.tmp` pot indica o operatie intrerupta. Pastreaza copii
  ale acestora, verifica backup-ul si restaureaza manual configuratia; nu le
  elimina automat presupunand ca sunt inutile.
- **Imaginea nu se schimba:** verifica activarea, incearca presetul Vibrant,
  apoi dezactiveaza corectia pentru comparatie. Compatibilitatea patch-urilor
  CMU cu firmware-ul exact ramane o verificare pe consola.

Pentru revenire, cu consola oprita, elimina numai `SwitchColor.ovl`, restaureaza
backup-ul modulului Fizeau anterior si configuratia salvata. Daca Fizeau a fost
instalat exclusiv pentru acest proiect, redenumeste temporar `boot2.flag` pentru
a-i opri pornirea automata. Nu sterge folderele generale `atmosphere` sau `switch`.

## Verificari pe consola

- Meniul se deschide in joc; sliderele schimba imaginea si B revine normal.
- Activarea/dezactivarea, resetarea si restaurarea functioneaza.
- Dupa salvare si repornire sunt pastrate toate cele patru profiluri.
- Suspendarea si revenirea din sleep nu blocheaza consola.
- Inchiderea overlay-ului lasa reglajele live active.

Nu sunt incluse sharpness spatial sau profiluri automate per joc.
