# SwitchColor 0.2.0 - ghid toolkit

Actualizarea adauga un meniu central, 18 preseturi, informatii despre consola,
control rapid si scurtaturi catre overlay-urile instalate. B revine cu un nivel;
inchiderea meniului elibereaza serviciile si opreste citirile periodice.

## Actualizare de la 0.1.0

Inchide SwitchColor. Din `SwitchColor-0.2.0-overlay-only.zip`, copiaza
`switch/.overlays/SwitchColor.ovl` in aceeasi locatie pe card, dupa ce pastrezi
o copie a versiunii anterioare. Redeschide-l din Tesla/Ultrahand. Backend-ul si
configuratia Fizeau nu necesita modificari pentru aceasta actualizare.

Combinatia detectata anterior pe consola utilizatorului: **L + apasare stick
drept + D-pad Jos**. Pentru o instalare noua foloseste `INSTALL_RO.md`.

## Preseturi

Deschide **Imagine si preseturi > Preseturi**, alege categoria, stilul si apoi
**Aplica live**. Detaliile arata valorile inainte de aplicare. Corectia imaginii
trebuie sa fie pornita pentru a vedea efectul. **Salveaza pentru repornire** din
meniul Imagine pastreaza alegerea pe SD.

| Categorie | Stiluri |
| --- | --- |
| Natural / stil OLED | Standard, Vibrant, OLED Soft, OLED Vivid, Deep Colors |
| Contrast / vizibilitate | Contrast+, Contrast Soft, Shadow Lift |
| Cald / cinema / seara | Cinema, Night, Warm 5500K, Warm+ 4500K, Amber 3200K, Reading |
| Stiluri creative | Cool 8000K, Retro Warm, Monochrome, Pastel |

OLED Soft/Vivid sunt stiluri pentru LCD; nu reproduc negrul fizic al unui panou
OLED. Sunt puncte de pornire vizuale, nu calibrari. Poti ajusta ulterior cele
sase slidere. Setarile sunt globale, nu selectate automat dupa joc. Preseturile
uniformizeaza zi/noapte, reseteaza filtrul/canalele RGB si opresc dimming-ul
Fizeau. Luminozitatea fizica a ecranului este in Control rapid.

## Informatii live

| Pagina | Informatii citite |
| --- | --- |
| Privire generala | Model, firmware, Title ID, baterie, SoC, IP local, spatiu SD liber |
| Baterie | Procent, incarcare bruta, estimare capacitate ramasa, incarcator, incarcare activa, tensiune, temperatura, limita curentului de incarcare |
| Temperaturi | SoC, PCB, carcasa/skin, baterie |
| Frecvente | CPU, GPU, memorie EMC, in MHz |
| Retea | IPv4 local, Wi-Fi activ, tipul conexiunii, starea internetului, semnal |
| Sistem | Modelul consolei, firmware detectat, Title ID activ |
| Stocare SD | Liber, total si ocupat, in GiB |

Pagina vizibila este recitita la aproximativ o secunda. A deschide valoarea
completa si codul erorii. Valorile inaccesibile apar ca **Indisponibil**.
Frecventa nu inseamna utilizare CPU/GPU sau FPS. Estimarea bateriei provine
din controler; limita curentului de incarcare nu este consumul instantaneu.
Informatiile si controlul rapid functioneaza independent de conexiunea Fizeau.

## Control rapid

- Luminozitate fizica: pasi de 5 puncte procentuale, intre 5 si 100%.
- Volum: un pas, pentru iesirea activa (difuzoare, casti etc.), in limitele
  raportate de sistem.

Numai apasarea comenzii modifica o valoare. Valorile sunt recitite dupa
scriere; o eroare de confirmare este afisata. Setarile automate de luminozitate
si dimming nu sunt modificate. Comenzile nu salveaza in configuratia Fizeau.

## Diagnostic si unelte instalate

**Exporta diagnostic pe SD** creeaza un fisier nou in
`config/SwitchColor/reports/`. Contine valorile disponibile si erorile, inclusiv
IP-ul local si Title ID. Raportul ramane pe card.

Scurtaturile deschid Status Monitor, FPSLocker, sys-clk si Sysmodules numai
daca fisierele lor sunt prezente. Sunt aplicatii separate, cu propriile
dependente. Lansarea inchide SwitchColor si lasa corectia imaginii activa.

## Directii pentru versiunile urmatoare

Acestea sunt propuneri; nu sunt implementate in 0.2.0:

1. Preseturi personale: salvare sub nume propriu, favorite, import/export si
   comparatie rapida A/B cu revenire la ultima imagine verificata.
2. Profiluri automate pe joc: legatura Title ID -> preset, cu serviciu de
   fundal pentru schimbarea profilului si dupa inchiderea overlay-ului.
3. Pagina de performanta cu FPS si frame time, folosind o sursa de masurare
   compatibila precum NX-FPS; grafice CPU/GPU si temperatura in aceeasi pagina.
4. Istoric de sesiune, inregistrari CSV si praguri configurabile de afisare
   pentru baterie/temperaturi, dupa verificarea senzorilor pe consola.
5. Aplicatie companion pe tot ecranul pentru editarea profilurilor si citirea
   rapoartelor; overlay-ul ramane accesul rapid din joc.

Sharpness spatial necesita alta integrare grafica decat Fizeau/CMU si nu este
promis ca un simplu slider. Nu sunt implementate modificari de ventilator,
limite de incarcare sau frecvente direct in SwitchColor 0.2.0.

Codul este compilat ARM64 si testat pe PC. Citirile serviciilor, comenzile
rapide, navigarea si lansarea uneltelor necesita verificare efectiva pe Switch.
