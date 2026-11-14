# Specyfikacja protokołu komunikacyjnego SmartFlasher

| | |
|---|---|
| **Tytuł dokumentu** | Specyfikacja protokołu komunikacyjnego SmartFlasher (SFCTL, SFVAR, SFINF) |
| **Wersja dokumentu** | 1.2 |
| **Data wydania** | 2026-11-14 |
| **Status** | Obowiązujący |
| **Dotyczy** | Urządzenie SmartFlasher – oprogramowanie układowe w wersji 0.9.x |

---

## Spis treści

1. [Wprowadzenie](#1-wprowadzenie)
2. [Architektura komunikacji](#2-architektura-komunikacji)
3. [Format ramki](#3-format-ramki)
4. [Model wymiany komunikatów](#4-model-wymiany-komunikatów)
5. [Zdanie SFCTL – sterowanie i konfiguracja](#5-zdanie-sfctl--sterowanie-i-konfiguracja)
6. [Parametry konfiguracyjne](#6-parametry-konfiguracyjne)
7. [Zdanie SFVAR – zmienne bieżące](#7-zdanie-sfvar--zmienne-bieżące)
8. [Zdanie SFINF – ramka informacyjna](#8-zdanie-sfinf--ramka-informacyjna)
9. [Lampa błyskowa – tryby, sekwencje i poziomy jasności](#9-lampa-błyskowa--tryby-sekwencje-i-poziomy-jasności)
10. [Kody błędów](#10-kody-błędów)
11. [Zdania wejściowe FLARM i GPS](#11-zdania-wejściowe-flarm-i-gps)
12. [Przykładowe sesje komunikacyjne](#12-przykładowe-sesje-komunikacyjne)
13. [Załącznik A – zestawienie ramek](#13-załącznik-a--zestawienie-ramek)
14. [Historia zmian dokumentu](#14-historia-zmian-dokumentu)

---

## 1. Wprowadzenie

### 1.1. Cel dokumentu

Niniejszy dokument stanowi specyfikację protokołu komunikacyjnego urządzenia SmartFlasher. Określa format ramek, zestaw poleceń i odpowiedzi, zasady kierowania komunikatów między portami urządzenia, a także znaczenie wszystkich pól przesyłanych w zdaniach własnych urządzenia. Dokument przeznaczony jest dla twórców oprogramowania współpracującego (aplikacje konfiguracyjne, wyświetlacze, stanowiska testowe) oraz dla personelu serwisowego.

### 1.2. Zakres

Specyfikacja obejmuje:

- zdanie **SFCTL** (*SmartFlasher Control*) – polecenia sterujące i konfiguracyjne oraz odpowiedzi urządzenia,
- zdanie **SFVAR** (*SmartFlasher Variables*) – odczyt bieżących wartości pomiarowych i stanu urządzenia,
- zdanie **SFINF** (*SmartFlasher Information*) – ramkę informacyjną wysyłaną samoczynnie przez urządzenie,
- zdania standardu FLARM oraz NMEA 0183 (GPS) rozpoznawane i interpretowane przez urządzenie.

### 1.3. Terminy i skróty

| Termin | Znaczenie |
|---|---|
| **Host** | Urządzenie lub aplikacja wysyłająca polecenia do SmartFlashera (komputer, wyświetlacz, aplikacja mobilna, stanowisko testowe). |
| **Urządzenie** | SmartFlasher. |
| **Zdanie (ramka)** | Pojedynczy komunikat w formacie NMEA 0183, zakończony znakami CR LF. |
| **ACK** | Potwierdzenie pozytywne – polecenie zostało wykonane. |
| **NACK** | Potwierdzenie negatywne – polecenie zostało odrzucone; zawiera nazwę odrzuconego polecenia i kod błędu. |
| **Port pełny** | Port, na którym urządzenie analizuje wszystkie zdania (port FLARM, port USB). |
| **Port kliencki** | Port, na którym urządzenie obsługuje lokalnie wyłącznie zdania SFCTL i SFVAR, a pozostałe dane przekazuje do FLARM (port DISPLAY, Bluetooth Classic, Bluetooth LE). |
| **Mostek (bridge)** | Funkcja przekazywania danych między portem FLARM a portami klienckimi. |
| **Tryb błysku** | Wzorzec czasowy pracy lampy błyskowej (0–4). |

### 1.4. Konwencje zapisu

- Ramki zapisywane są czcionką o stałej szerokości, np. `$SFCTL,LIST*60`.
- Znaki końca linii CR LF (0x0D 0x0A) są obowiązkowe na końcu każdej ramki, lecz w przykładach nie są pokazywane.
- `<NAZWA>` oznacza pole, w którego miejsce należy wstawić właściwą wartość.
- `[ ... ]` oznacza element opcjonalny.
- `*hh` oznacza dwucyfrową szesnastkową sumę kontrolną w zapisie ogólnym. We wszystkich przykładach konkretnych ramek suma kontrolna jest podana w postaci obliczonej i poprawnej.
- Strzałka `→` oznacza kierunek Host → Urządzenie, strzałka `←` kierunek Urządzenie → Host.

---

## 2. Architektura komunikacji

### 2.1. Porty komunikacyjne

Urządzenie przyjmuje zdania protokołu na pięciu niezależnych portach. Każdy port posiada własny analizator składni, dzięki czemu dane odbierane na jednym porcie nie mogą zakłócić zdania odbieranego na innym.

| Port | Połączenie fizyczne | Parametry transmisji | Rodzaj portu | Obsługiwane zdania |
|---|---|---|---|---|
| **FLARM** | Gniazdo RJ45 „FLARM”, RS-232 | 8N1, prędkość wg parametru `FLARM_BAUD` (domyślnie 19200 bit/s) | pełny (port podstawowy) | wszystkie: FLARM, GPS, SFCTL, SFVAR |
| **DISPLAY** | Gniazdo RJ45 „DISPLAY”, RS-232 | 8N1, prędkość wg parametru `DISPLAY_BAUD` (domyślnie 19200 bit/s) | kliencki | SFCTL i SFVAR obsługiwane lokalnie; pozostałe dane przekazywane do FLARM |
| **Bluetooth Classic** | Profil SPP, nazwa urządzenia = parametr `DEVICE_NAME` | – | kliencki | jak DISPLAY |
| **Bluetooth LE** | Usługa NUS (Nordic UART Service), nazwa urządzenia = parametr `DEVICE_NAME` | – | kliencki | jak DISPLAY |
| **USB** | Złącze USB-C (port serwisowy) | 115200 bit/s, 8N1 | pełny | wszystkie: FLARM, GPS, SFCTL, SFVAR |

Aktywny rodzaj łącza Bluetooth wybierany jest parametrem `BT_MODE` (`OFF`, `CLASSIC`, `BLE`).

### 2.2. Kierowanie odpowiedzi

1. Odpowiedzi na zdania SFCTL i SFVAR (`ACK`, `NACK`, wiersze danych `STATUS`, `STATS`, listy parametrów, odpowiedzi `$SFVAR`) wysyłane są **wyłącznie na port, z którego nadeszło polecenie**.
2. Pozostałe ramki generowane przez urządzenie – w tym ramki `$PFLAI` wywołane poleceniem `EVENT` oraz ramki informacyjne `$SFINF` – wysyłane są **zawsze na port FLARM**, a gdy włączony jest mostek, również na porty klienckie.

### 2.3. Filtrowanie danych na portach klienckich

Na portach klienckich (DISPLAY, Bluetooth Classic, Bluetooth LE) urządzenie stosuje następujące zasady:

1. Każdy odebrany bajt, który nie może stanowić początku zdania `$SFCTL` lub `$SFVAR`, jest niezwłocznie przekazywany do portu FLARM. Urządzenie wstrzymuje co najwyżej 6 pierwszych bajtów linii, do czasu rozstrzygnięcia, czy jest to zdanie sterujące.
2. Dzięki temu zdania konfiguracyjne FLARM oraz dane niebędące zdaniami NMEA przechodzą przez urządzenie w niezmienionej postaci.
3. Zdania `$SFCTL` i `$SFVAR` są obsługiwane lokalnie i **nigdy nie są przekazywane do FLARM**.
4. Linie sterujące (rozpoczynające się od `$SFCTL` lub `$SFVAR`) dłuższe niż 128 bajtów są odrzucane w całości i nie są przekazywane dalej.
5. Przekazywanie danych do FLARM odbywa się tylko wtedy, gdy:
   - dla portu DISPLAY: włączone są parametry `BRIDGE_ENABLED` oraz `BRIDGE_TWO_WAY`,
   - dla portów Bluetooth: mostek Bluetooth jest aktywny, klient jest połączony oraz włączony jest parametr `BRIDGE_TWO_WAY`.

   W pozostałych przypadkach dane inne niż SFCTL/SFVAR są odrzucane.

### 2.4. Mostkowanie danych FLARM

Zdania rozpoznane na portach pełnych (FLARM, USB), z wyjątkiem SFCTL i SFVAR, są retransmitowane:

- na port DISPLAY – gdy włączony jest parametr `BRIDGE_ENABLED`,
- na aktywny port Bluetooth (Classic lub LE) – gdy aktywny jest mostek Bluetooth.

Zdania SFCTL i SFVAR **nigdy nie są mostkowane**.

---

## 3. Format ramki

### 3.1. Struktura zdania

Wszystkie zdania protokołu mają format zgodny z normą NMEA 0183:

```
$<TYP>,<pole1>,<pole2>,...,<poleN>*hh<CR><LF>
```

| Element | Opis |
|---|---|
| `$` | Znak początku zdania (0x24). |
| `<TYP>` | Identyfikator zdania, od 1 do 5 znaków alfanumerycznych, np. `SFCTL`, `SFVAR`, `SFINF`, `PFLAU`, `GPRMC`. |
| `,` | Separator pól (0x2C). |
| `<pole>` | Zawartość pola w postaci tekstowej ASCII. Pole może być puste. |
| `*` | Ogranicznik sumy kontrolnej (0x2A). |
| `hh` | Suma kontrolna – dwie cyfry szesnastkowe. |
| `<CR><LF>` | Zakończenie zdania (0x0D 0x0A). |

### 3.2. Suma kontrolna

Suma kontrolna jest wynikiem operacji XOR (alternatywy wykluczającej) wykonanej kolejno na kodach ASCII wszystkich znaków znajdujących się **pomiędzy** znakiem `$` a znakiem `*` (z wyłączeniem obu tych znaków). Wynik zapisywany jest jako dwie cyfry szesnastkowe.

- Urządzenie generuje sumy kontrolne wielkimi literami (`A`–`F`).
- Urządzenie akceptuje w odbieranych zdaniach cyfry szesnastkowe zapisane zarówno wielkimi, jak i małymi literami.
- Suma kontrolna jest **obowiązkowa**. Zdania bez sumy kontrolnej lub z niepoprawną sumą są odrzucane bez odpowiedzi.

**Przykład obliczenia** dla zdania `SFCTL,RESET`:

| Krok | Znak | Kod ASCII | Wynik XOR |
|---|---|---|---|
| 1 | `S` | 0x53 | 0x53 |
| 2 | `F` | 0x46 | 0x15 |
| 3 | `C` | 0x43 | 0x56 |
| 4 | `T` | 0x54 | 0x02 |
| 5 | `L` | 0x4C | 0x4E |
| 6 | `,` | 0x2C | 0x62 |
| 7 | `R` | 0x52 | 0x30 |
| 8 | `E` | 0x45 | 0x75 |
| 9 | `S` | 0x53 | 0x26 |
| 10 | `E` | 0x45 | 0x63 |
| 11 | `T` | 0x54 | **0x37** |

Kompletna ramka: `$SFCTL,RESET*37`

### 3.3. Ograniczenia

1. Długość zdania przyjmowanego przez urządzenie nie może przekraczać **82 znaków** łącznie ze znakiem `$`, sumą kontrolną i znakami CR LF (co odpowiada 76 znakom pomiędzy `$` a `*`). Dłuższe zdania są odrzucane.
2. Zdanie musi być zakończone parą znaków CR LF. Inna sekwencja końca linii powoduje odrzucenie zdania.
3. Znaki `$`, `*`, `,` oraz znaki CR i LF są zastrzeżone i nie mogą występować wewnątrz pól (w szczególności w nazwach i wartościach parametrów).
4. Zdania wysyłane przez urządzenie zakończone są parą znaków CR LF.

### 3.4. Obsługa zdań błędnych

Zdania z niepoprawną sumą kontrolną, niepoprawną składnią lub nierozpoznanym identyfikatorem typu są odrzucane bez wysyłania odpowiedzi. Na portach pełnych (FLARM, USB) każde takie zdanie zwiększa licznik `INVALID` raportowany przez polecenie `STATS`.

### 3.5. Wielkość liter

| Element | Wielkość liter |
|---|---|
| Identyfikator zdania (`SFCTL`, `SFVAR`) | rozróżniana – wyłącznie wielkie litery |
| Słowo polecenia SFCTL (`LIST`, `R`, `W`, `STATUS`, …) | rozróżniana – wyłącznie wielkie litery |
| Nazwy parametrów (polecenia `R`, `W`) | rozróżniana – wyłącznie wielkie litery |
| Akcje polecenia `FLASH` (`ON`, `MODE`, …) | nierozróżniana |
| Cel i stan polecenia `SWITCH` | nierozróżniana (w odpowiedzi zamieniane na wielkie litery) |
| Nazwy zmiennych SFVAR | nierozróżniana (w odpowiedzi zamieniane na wielkie litery) |
| Wartości parametrów tekstowych | rozróżniana – zapisywane bez zmian |

---

## 4. Model wymiany komunikatów

1. Komunikacja ma charakter **zapytanie – odpowiedź**. Host wysyła polecenie, urządzenie odpowiada na port, z którego polecenie nadeszło.
2. Każde poprawne polecenie SFCTL kończy się dokładnie jedną ramką `ACK` lub `NACK`.
3. Polecenia zwracające wiele wartości (`LIST`, `STATUS`, `STATS`, `FLASH,STATUS`) wysyłają najpierw wiersze danych, a następnie **zamykającą ramkę `ACK`**. Odebranie ramki `ACK` oznacza koniec odpowiedzi.
4. Odpowiedź na zdanie SFVAR nie zawiera ramki `ACK`; jest nią jedna lub kilka ramek `$SFVAR` (rozdział 7).
5. Host powinien wysyłać kolejne polecenie dopiero po odebraniu odpowiedzi na polecenie poprzednie.
6. Brak jakiejkolwiek odpowiedzi oznacza, że zdanie zostało odrzucone na poziomie ramki (rozdział 3.4). Zaleca się, aby host stosował limit czasu oczekiwania na odpowiedź nie krótszy niż 1 sekunda i w razie jego przekroczenia ponawiał polecenie.
7. Na portach pełnych odpowiedzi mogą być przeplatane innymi zdaniami (np. danymi FLARM lub ramkami `$SFINF`). Host powinien identyfikować odpowiedzi po identyfikatorze zdania.

---

## 5. Zdanie SFCTL – sterowanie i konfiguracja

### 5.1. Format ogólny

Polecenie (Host → Urządzenie):

```
$SFCTL,<POLECENIE>[,<argument1>[,<argument2>]]*hh
```

Odpowiedzi (Urządzenie → Host):

```
$SFCTL,ACK,<POLECENIE lub NAZWA>[,<wartość>...]*hh
$SFCTL,NACK,<POLECENIE>,<KOD_BŁĘDU>*hh
```

**Format odpowiedzi negatywnej.** Ramka `NACK` ma zawsze stałą liczbę pól, co pozwala jednoznacznie przypisać ją do odrzuconego polecenia:

| Pole | Zawartość |
|---|---|
| 1 | `NACK` |
| 2 | `<POLECENIE>` – słowo polecenia z odrzuconego żądania, w postaci w jakiej zostało odebrane (np. `R`, `W`, `FLASH`, `SWITCH`). W przypadku polecenia nierozpoznanego pole zawiera odebraną nazwę polecenia. |
| 3 | `<KOD_BŁĘDU>` – przyczyna odrzucenia (rozdział 10). |

Przykład: odpowiedzią na `$SFCTL,SWITCH*70` jest `$SFCTL,NACK,SWITCH,MISSING_PARAMS*70`.

### 5.2. Zestawienie poleceń

| Polecenie | Opis | Odpowiedź pozytywna | Rozdział |
|---|---|---|---|
| `LIST` | Lista wszystkich parametrów z bieżącymi wartościami | wiersze parametrów + `ACK,LIST` | 5.3 |
| `R` | Odczyt parametru | `ACK,<NAZWA>,<WARTOŚĆ>` | 5.4 |
| `W` | Zapis parametru | `ACK,<NAZWA>,<WARTOŚĆ>` | 5.5 |
| `DEFAULTS` | Przywrócenie ustawień fabrycznych | `ACK,DEFAULTS` | 5.6 |
| `STATUS` | Stan systemu | wiersze `STATUS` + `ACK,STATUS,COMPLETE` | 5.7 |
| `STATS` | Statystyki odebranych zdań | wiersze `STATS` + `ACK,STATS,COMPLETE` | 5.8 |
| `RESET` | Ponowne uruchomienie urządzenia | `ACK,RESET,RESTARTING` | 5.9 |
| `SYSINFO` | Informacje systemowe w postaci tekstowej | `ACK,SYSINFO,<tekst>` | 5.10 |
| `FLASH` | Sterowanie lampą błyskową | zależna od akcji | 5.11 |
| `SWITCH` | Bezpośrednie przełączanie wyjść sprzętowych | `ACK,SWITCH,<CEL>,<STAN>` | 5.12 |
| `EVENT` | Zgłoszenie zdarzenia pilota do FLARM | `ACK,EVENT` | 5.13 |

Polecenie o nierozpoznanej nazwie powoduje odpowiedź `$SFCTL,NACK,<nazwa>,UNKNOWN*hh`, np. `$SFCTL,PING*72` → `$SFCTL,NACK,PING,UNKNOWN*3D`.

### 5.3. LIST – lista parametrów

**Polecenie:**

```
$SFCTL,LIST*60
```

**Odpowiedź:** dla każdego parametru konfiguracyjnego, w kolejności określonej w rozdziale 6, jedna ramka w formacie:

```
$SFCTL,<NAZWA>,<WARTOŚĆ>*hh
```

a następnie ramka zamykająca `$SFCTL,ACK,LIST*05`.

**Przykład** (urządzenie z ustawieniami fabrycznymi):

```
→ $SFCTL,LIST*60
← $SFCTL,DEVICE_NAME,SmartFlasher*10
← $SFCTL,MODE,STANDALONE*48
← $SFCTL,POWER_LEVEL,35*1E
← $SFCTL,FLARM_BAUD,19200*6D
← $SFCTL,FLARM_POWER,false*67
← $SFCTL,DISPLAY_BAUD,19200*63
← $SFCTL,DISPLAY_POWER,false*69
← $SFCTL,BT_MODE,OFF*4B
← $SFCTL,BRIDGE_ENABLED,false*36
← $SFCTL,BRIDGE_TWO_WAY,true*44
← $SFCTL,OTA_URL,https://smartflasher.eu/firmware/manifest*6F
← $SFCTL,DEBUG_FRAMES,false*33
← $SFCTL,SFINF_PERIODIC,true*5C
← $SFCTL,ACK,LIST*05
```

### 5.4. R – odczyt parametru

**Polecenie:**

```
$SFCTL,R,<NAZWA>*hh
```

**Odpowiedź pozytywna:**

```
$SFCTL,ACK,<NAZWA>,<WARTOŚĆ>*hh
```

**Odpowiedź negatywna:** `$SFCTL,NACK,R,UNKNOWN*7F` – parametr o podanej nazwie nie istnieje.

**Przykłady:**

```
→ $SFCTL,R,POWER_LEVEL*4A
← $SFCTL,ACK,POWER_LEVEL,35*7B

→ $SFCTL,R,DEVICE_NAME*5C
← $SFCTL,ACK,DEVICE_NAME,SmartFlasher*75

→ $SFCTL,R,MODE*1F
← $SFCTL,ACK,MODE,STANDALONE*2D
```

### 5.5. W – zapis parametru

**Polecenie:**

```
$SFCTL,W,<NAZWA>,<WARTOŚĆ>*hh
```

Pole `<NAZWA>` jest obowiązkowe. Pominięcie pola `<WARTOŚĆ>` oznacza zapis wartości pustej, dopuszczalnej wyłącznie dla parametrów tekstowych.

**Odpowiedź pozytywna** – zawiera wartość parametru **po zapisie**:

```
$SFCTL,ACK,<NAZWA>,<WARTOŚĆ>*hh
```

**Odpowiedzi negatywne:**

| Ramka | Znaczenie |
|---|---|
| `$SFCTL,NACK,W,INVALID_VALUE*77` | Brak nazwy parametru (np. `$SFCTL,W*35`) lub wartość niedozwolona dla parametru logicznego albo wyliczeniowego. |
| `$SFCTL,NACK,W,UNKNOWN*7A` | Parametr o podanej nazwie nie istnieje. |
| `$SFCTL,NACK,W,READ_ONLY*6B` | Parametr jest tylko do odczytu. |

Po odpowiedzi negatywnej wartość parametru pozostaje bez zmian.

**Zasady interpretacji wartości:**

| Typ parametru | Interpretacja wartości |
|---|---|
| tekstowy | Wartość zapisywana jest bez zmian. |
| logiczny | Dozwolone wartości: `true` lub `1` (prawda), `false` lub `0` (fałsz); wielkość liter jest rozróżniana. Inna wartość jest odrzucana odpowiedzią `$SFCTL,NACK,W,INVALID_VALUE*77`. Urządzenie zwraca `true` / `false`. |
| wyliczeniowy | Wartość musi być dokładnie równa jednej z dozwolonych wartości (rozdział 6). Wartość spoza listy jest odrzucana odpowiedzią `$SFCTL,NACK,W,INVALID_VALUE*77`. |

Host powinien zawsze porównać wartość zwróconą w ramce `ACK` z wartością zapisywaną.

Zapisana wartość jest natychmiast utrwalana w pamięci nieulotnej i natychmiast stosowana (np. zmiana `POWER_LEVEL` zmienia jasność lampy, zmiana `FLARM_BAUD` przestawia prędkość portu FLARM).

**Przykłady:**

```
→ $SFCTL,W,POWER_LEVEL,40*67
← $SFCTL,ACK,POWER_LEVEL,40*79

→ $SFCTL,W,BRIDGE_ENABLED,true*26
← $SFCTL,ACK,BRIDGE_ENABLED,true*38

→ $SFCTL,W,MODE,FLARM*62
← $SFCTL,ACK,MODE,FLARM*7C

→ $SFCTL,W,POWER_LEVEL,50*66
← $SFCTL,NACK,W,INVALID_VALUE*77        (wartość spoza listy – parametr bez zmian)

→ $SFCTL,W,VOLUME,5*08
← $SFCTL,NACK,W,UNKNOWN*7A
```

> **Uwaga:** zmiana prędkości portu (`FLARM_BAUD`, `DISPLAY_BAUD`) wysłana przez ten sam port powoduje, że kolejne zdania muszą być wysyłane z nową prędkością.

### 5.6. DEFAULTS – przywrócenie ustawień fabrycznych

**Polecenie:**

```
$SFCTL,DEFAULTS*7A
```

**Odpowiedź:**

```
$SFCTL,ACK,DEFAULTS*1F
```

Wszystkie parametry przyjmują wartości domyślne (rozdział 6), które są natychmiast zapisywane w pamięci nieulotnej i stosowane. Operacja jest nieodwracalna.

### 5.7. STATUS – stan systemu

**Polecenie:**

```
$SFCTL,STATUS*76
```

**Odpowiedź:** sekwencja ramek `$SFCTL,STATUS,<KLUCZ>,<WARTOŚĆ>` w podanej niżej kolejności, zakończona ramką `$SFCTL,ACK,STATUS,COMPLETE*36`.

| Klucz | Jednostka | Opis |
|---|---|---|
| `UPTIME` | s | Czas od uruchomienia urządzenia. |
| `FRAMES_COUNT` | – | Łączna liczba rozpoznanych zdań od uruchomienia. |
| `FRAMES_PER_SEC` | zdania/s | Bieżąca liczba zdań na sekundę (2 miejsca po przecinku). |
| `HEAP` | B | Wolna pamięć operacyjna. |
| `WIFI_CLIENTS` | – | Liczba klientów połączonych z punktem dostępowym Wi-Fi urządzenia. |
| `FLASH_MODE` | – | Bieżący tryb błysku (0–4). |
| `AIRBORNE` | – | Stan lotu: `1` – w locie, `0` – na ziemi. |

**Przykład:**

```
→ $SFCTL,STATUS*76
← $SFCTL,STATUS,UPTIME,3725*65
← $SFCTL,STATUS,FRAMES_COUNT,15342*55
← $SFCTL,STATUS,FRAMES_PER_SEC,4.12*73
← $SFCTL,STATUS,HEAP,182344*62
← $SFCTL,STATUS,WIFI_CLIENTS,0*42
← $SFCTL,STATUS,FLASH_MODE,1*4B
← $SFCTL,STATUS,AIRBORNE,1*49
← $SFCTL,ACK,STATUS,COMPLETE*36
```

### 5.8. STATS – statystyki odebranych zdań

**Polecenie:**

```
$SFCTL,STATS*23
```

**Odpowiedź:** sekwencja ramek `$SFCTL,STATS,<KLUCZ>,<WARTOŚĆ>` zakończona ramką `$SFCTL,ACK,STATS,COMPLETE*63`. Kolejność kluczy:

1. `TOTAL` – łączna liczba rozpoznanych zdań,
2. `FPS` – bieżąca liczba zdań na sekundę (2 miejsca po przecinku),
3. liczniki poszczególnych typów zdań, w porządku alfabetycznym; raportowane są wyłącznie typy, które zostały odebrane co najmniej raz (np. `GPGGA`, `GPRMC`, `PFLAA`, `PFLAU`, `SFCTL`, `SFVAR`),
4. `INVALID` – liczba zdań odrzuconych na portach pełnych (rozdział 3.4).

Liczniki obejmują zdania odebrane na wszystkich portach, przy czym na portach klienckich zliczane są wyłącznie zdania SFCTL i SFVAR. Samo polecenie `STATS` jest uwzględniane w liczniku `SFCTL`.

**Przykład:**

```
→ $SFCTL,STATS*23
← $SFCTL,STATS,TOTAL,13392*5B
← $SFCTL,STATS,FPS,4.12*7F
← $SFCTL,STATS,GPGGA,3050*73
← $SFCTL,STATS,GPGSA,3050*67
← $SFCTL,STATS,GPRMC,3050*6E
← $SFCTL,STATS,PFLAA,1180*71
← $SFCTL,STATS,PFLAU,3050*6B
← $SFCTL,STATS,SFCTL,12*6E
← $SFCTL,STATS,INVALID,3*41
← $SFCTL,ACK,STATS,COMPLETE*63
```

### 5.9. RESET – ponowne uruchomienie

**Polecenie:**

```
$SFCTL,RESET*37
```

**Odpowiedź:**

```
$SFCTL,ACK,RESET,RESTARTING*69
```

Po wysłaniu potwierdzenia urządzenie odczekuje 100 ms, a następnie wykonuje ponowne uruchomienie. Stan nieutrwalony (np. zmiany wykonane poleceniami `FLASH` i `SWITCH`, statystyki) zostaje utracony; parametry konfiguracyjne są zachowane. Połączenia Bluetooth zostają przerwane.

### 5.10. SYSINFO – informacje systemowe

**Polecenie:**

```
$SFCTL,SYSINFO*35
```

**Odpowiedź:**

```
$SFCTL,ACK,SYSINFO,<blok tekstu>*hh
```

Pole `<blok tekstu>` zawiera wielowierszowy raport tekstowy (wiersze rozdzielone znakami CR LF) przeznaczony do odczytu przez człowieka na terminalu. Ramka ta przekracza ograniczenia długości zdania NMEA i nie jest przeznaczona do automatycznej analizy; suma kontrolna obejmuje cały blok tekstu wraz ze znakami CR LF. Do automatycznego odczytu informacji o urządzeniu należy stosować zdanie SFVAR (rozdział 7).

Raport zawiera:

| Sekcja | Informacje |
|---|---|
| Sprzęt | model i rewizja układu, liczba rdzeni i częstotliwość procesora, adres MAC, numer seryjny, rozmiar i prędkość pamięci Flash |
| Pamięć | rozmiar programu i wolne miejsce na program, wolna/całkowita pamięć operacyjna, zajętość systemu plików |
| Oprogramowanie | wersja oprogramowania SmartFlasher, data kompilacji, wersja SDK |
| Sieć | adres IP punktu dostępowego, liczba klientów Wi-Fi, czas pracy |

**Przykładowa zawartość bloku tekstu:**

```
========================================
       SYSTEM INFORMATION
========================================
Chip: ESP32 Rev 1
Cores: 2 @ 240 MHz
MAC: AA:BB:CC:DD:EE:FF
Serial: 2532001
Flash: 8.00 MB @ 40 MHz

Memory:
  Sketch: 1.70 MB / Free: 1.48 MB
  Heap: 239.98 KB / 320.00 KB
  FS: 64.00 KB / 256.00 KB

Software:
  SmartFlasher v0.9.106
  Build: 2026-05-06 01:28
  SDK: v4.4.6

Network:
  AP IP: 192.168.4.1
  Clients: 0
  Uptime: 1h 2m 5s
========================================
```

### 5.11. FLASH – sterowanie lampą błyskową

**Format ogólny:**

```
$SFCTL,FLASH,<AKCJA>[,<argument>]*hh
```

Zmiany wykonane poleceniem `FLASH` nie są zapisywane w pamięci nieulotnej. Trwałe ustawienie jasności lampy realizuje parametr `POWER_LEVEL` (rozdział 6). Znaczenie trybów błysku i poziomów jasności opisano w rozdziale 9.

| Akcja | Argument | Opis | Odpowiedź pozytywna |
|---|---|---|---|
| `ON` | – | Włączenie lampy | `$SFCTL,ACK,FLASH,ON*7A` |
| `OFF` | – | Wyłączenie lampy | `$SFCTL,ACK,FLASH,OFF*34` |
| `MODE` | 0–4 | Ustawienie trybu błysku | `$SFCTL,ACK,FLASH_MODE,<tryb>*hh` |
| `BRIGHTNESS` | 0–5 | Ustawienie poziomu jasności | `$SFCTL,ACK,FLASH_BRIGHTNESS,<poziom>*hh` |
| `TEST` | – | Uruchomienie sekwencji testowej | `$SFCTL,ACK,FLASH_TEST,STARTED*73` |
| `STATUS` | – | Odczyt stanu lampy | wiersze `STATUS` + `$SFCTL,ACK,FLASH_STATUS,COMPLETE*39` |

#### 5.11.1. FLASH,ON

```
→ $SFCTL,FLASH,ON*1F
← $SFCTL,ACK,FLASH,ON*7A
```

Włącza lampę. Jeżeli ustawiony jest tryb 1–4, lampa rozpoczyna pracę w tym trybie (w trybie pracy FLARM – wyłącznie w locie, zob. rozdział 9.3). Jeżeli ustawiony jest tryb 0, lampa świeci światłem ciągłym z bieżącą jasnością.

#### 5.11.2. FLASH,OFF

```
→ $SFCTL,FLASH,OFF*51
← $SFCTL,ACK,FLASH,OFF*34
```

Wyłącza lampę i zatrzymuje bieżącą sekwencję. Ustawiony tryb błysku i jasność zostają zachowane do ponownego włączenia.

#### 5.11.3. FLASH,MODE

```
$SFCTL,FLASH,MODE,<tryb>*hh
```

| Ramka | Tryb |
|---|---|
| `$SFCTL,FLASH,MODE,0*01` | 0 – brak błysku |
| `$SFCTL,FLASH,MODE,1*00` | 1 – tryb podstawowy |
| `$SFCTL,FLASH,MODE,2*03` | 2 – alarm niski |
| `$SFCTL,FLASH,MODE,3*02` | 3 – alarm ważny |
| `$SFCTL,FLASH,MODE,4*05` | 4 – alarm pilny |

Odpowiedź pozytywna potwierdza żądany tryb, np. `$SFCTL,ACK,FLASH_MODE,2*15`.

Odpowiedzi negatywne: `$SFCTL,NACK,FLASH,MISSING_MODE*2D` (brak argumentu), `$SFCTL,NACK,FLASH,INVALID_MODE*38` (wartość spoza zakresu 0–4).

Tryb 0 zatrzymuje sekwencję, lecz nie wyłącza lampy – po ustawieniu trybu 1–4 lampa wznawia pracę. W trybie pracy STANDALONE każdy tryb 1–4 jest realizowany jako tryb 1 (rozdział 9.3); ramka `ACK` zawiera wówczas tryb żądany, a tryb faktycznie realizowany można odczytać akcją `STATUS`.

#### 5.11.4. FLASH,BRIGHTNESS

```
$SFCTL,FLASH,BRIGHTNESS,<poziom>*hh
```

| Ramka | Poziom |
|---|---|
| `$SFCTL,FLASH,BRIGHTNESS,0*0B` | 0 – 20 % |
| `$SFCTL,FLASH,BRIGHTNESS,1*0A` | 1 – 27 % |
| `$SFCTL,FLASH,BRIGHTNESS,2*09` | 2 – 35 % |
| `$SFCTL,FLASH,BRIGHTNESS,3*08` | 3 – 40 % |
| `$SFCTL,FLASH,BRIGHTNESS,4*0F` | 4 – 45 % |
| `$SFCTL,FLASH,BRIGHTNESS,5*0E` | 5 – 100 % |

Odpowiedź pozytywna, np. `$SFCTL,ACK,FLASH_BRIGHTNESS,3*1E`.

Odpowiedzi negatywne: `$SFCTL,NACK,FLASH,MISSING_BRIGHTNESS*27` (brak argumentu), `$SFCTL,NACK,FLASH,INVALID_BRIGHTNESS*32` (wartość spoza zakresu 0–5).

#### 5.11.5. FLASH,TEST

```
→ $SFCTL,FLASH,TEST*08
← $SFCTL,ACK,FLASH_TEST,STARTED*73
```

Uruchamia 30-sekundową sekwencję testową (rozdział 9.5) niezależnie od stanu lampy i trybu pracy. Bieżący tryb zostaje wstrzymany i wznowiony po zakończeniu testu. Na czas testu pole `testFlash` ramki `$SFINF` przyjmuje wartość 1.

Odpowiedź negatywna: `$SFCTL,NACK,FLASH,TEST_ALREADY_ACTIVE*69` – sekwencja testowa jest już w toku.

#### 5.11.6. FLASH,STATUS

```
→ $SFCTL,FLASH,STATUS*0A
← $SFCTL,STATUS,FLASH_ON,1*49
← $SFCTL,STATUS,FLASH_MODE,1*4B
← $SFCTL,STATUS,FLASH_BRIGHTNESS,2*42
← $SFCTL,STATUS,FLASH_TEST,0*5F
← $SFCTL,ACK,FLASH_STATUS,COMPLETE*39
```

| Klucz | Wartości | Opis |
|---|---|---|
| `FLASH_ON` | 0 / 1 | Lampa wyłączona / włączona. |
| `FLASH_MODE` | 0–4 | Bieżący tryb błysku. |
| `FLASH_BRIGHTNESS` | 0–5 | Bieżący poziom jasności. |
| `FLASH_TEST` | 0 / 1 | Sekwencja testowa nieaktywna / w toku. |

#### 5.11.7. Błędy polecenia FLASH

| Ramka | Przyczyna |
|---|---|
| `$SFCTL,NACK,FLASH,INVALID_FLASH_CMD*7E` | Brak akcji, np. `$SFCTL,FLASH*32`. |
| `$SFCTL,NACK,FLASH,UNKNOWN_FLASH_ACTION*33` | Nierozpoznana akcja. |
| `$SFCTL,NACK,FLASH,MISSING_MODE*2D` | Akcja `MODE` bez argumentu. |
| `$SFCTL,NACK,FLASH,INVALID_MODE*38` | Tryb spoza zakresu 0–4. |
| `$SFCTL,NACK,FLASH,MISSING_BRIGHTNESS*27` | Akcja `BRIGHTNESS` bez argumentu. |
| `$SFCTL,NACK,FLASH,INVALID_BRIGHTNESS*32` | Poziom spoza zakresu 0–5. |
| `$SFCTL,NACK,FLASH,TEST_ALREADY_ACTIVE*69` | Sekwencja testowa jest już w toku. |

### 5.12. SWITCH – przełączanie wyjść sprzętowych

**Format:**

```
$SFCTL,SWITCH,<CEL>,<STAN>*hh
```

gdzie `<STAN>` przyjmuje wartość `ON` lub `OFF`.

**Odpowiedź pozytywna:**

```
$SFCTL,ACK,SWITCH,<CEL>,<STAN>*hh
```

Polecenie przeznaczone jest do celów serwisowych i testowych. Zmiany nie są zapisywane w pamięci nieulotnej – trwałe sterowanie zasilaniem portów realizują parametry `FLARM_POWER` i `DISPLAY_POWER`.

| Cel | Opis | Ramka włączenia | Ramka wyłączenia |
|---|---|---|---|
| `FLARM_PORT_POWER` | Zasilanie portu FLARM | `$SFCTL,SWITCH,FLARM_PORT_POWER,ON*63` | `$SFCTL,SWITCH,FLARM_PORT_POWER,OFF*2D` |
| `DISPLAY_PORT_POWER` | Zasilanie portu DISPLAY | `$SFCTL,SWITCH,DISPLAY_PORT_POWER,ON*6D` | `$SFCTL,SWITCH,DISPLAY_PORT_POWER,OFF*23` |
| `FLARM_LED_YELLOW` | Żółta dioda portu FLARM | `$SFCTL,SWITCH,FLARM_LED_YELLOW,ON*6C` | `$SFCTL,SWITCH,FLARM_LED_YELLOW,OFF*22` |
| `DISPLAY_LED_YELLOW` | Żółta dioda portu DISPLAY | `$SFCTL,SWITCH,DISPLAY_LED_YELLOW,ON*62` | `$SFCTL,SWITCH,DISPLAY_LED_YELLOW,OFF*2C` |
| `POWER_LED` | Dioda zasilania | `$SFCTL,SWITCH,POWER_LED,ON*3C` | `$SFCTL,SWITCH,POWER_LED,OFF*72` |
| `EXT_LED` | Dioda zewnętrzna | `$SFCTL,SWITCH,EXT_LED,ON*2A` | `$SFCTL,SWITCH,EXT_LED,OFF*64` |
| `BT_LED` | Dioda Bluetooth | `$SFCTL,SWITCH,BT_LED,ON*75` | `$SFCTL,SWITCH,BT_LED,OFF*3B` |
| `FLASH_LED` | Lampa błyskowa (działanie jak `FLASH,ON` / `FLASH,OFF`) | `$SFCTL,SWITCH,FLASH_LED,ON*33` | `$SFCTL,SWITCH,FLASH_LED,OFF*7D` |

Odpowiedzi pozytywne odpowiadające ramkom włączenia i wyłączenia:

| Cel | Odpowiedź na ON | Odpowiedź na OFF |
|---|---|---|
| `FLARM_PORT_POWER` | `$SFCTL,ACK,SWITCH,FLARM_PORT_POWER,ON*06` | `$SFCTL,ACK,SWITCH,FLARM_PORT_POWER,OFF*48` |
| `DISPLAY_PORT_POWER` | `$SFCTL,ACK,SWITCH,DISPLAY_PORT_POWER,ON*08` | `$SFCTL,ACK,SWITCH,DISPLAY_PORT_POWER,OFF*46` |
| `FLARM_LED_YELLOW` | `$SFCTL,ACK,SWITCH,FLARM_LED_YELLOW,ON*09` | `$SFCTL,ACK,SWITCH,FLARM_LED_YELLOW,OFF*47` |
| `DISPLAY_LED_YELLOW` | `$SFCTL,ACK,SWITCH,DISPLAY_LED_YELLOW,ON*07` | `$SFCTL,ACK,SWITCH,DISPLAY_LED_YELLOW,OFF*49` |
| `POWER_LED` | `$SFCTL,ACK,SWITCH,POWER_LED,ON*59` | `$SFCTL,ACK,SWITCH,POWER_LED,OFF*17` |
| `EXT_LED` | `$SFCTL,ACK,SWITCH,EXT_LED,ON*4F` | `$SFCTL,ACK,SWITCH,EXT_LED,OFF*01` |
| `BT_LED` | `$SFCTL,ACK,SWITCH,BT_LED,ON*10` | `$SFCTL,ACK,SWITCH,BT_LED,OFF*5E` |
| `FLASH_LED` | `$SFCTL,ACK,SWITCH,FLASH_LED,ON*56` | `$SFCTL,ACK,SWITCH,FLASH_LED,OFF*18` |

Wielkość liter celu i stanu nie jest rozróżniana; w odpowiedzi są one zapisywane wielkimi literami:

```
→ $SFCTL,SWITCH,ext_led,on*2A
← $SFCTL,ACK,SWITCH,EXT_LED,ON*4F
```

**Odpowiedzi negatywne:**

| Ramka | Przyczyna |
|---|---|
| `$SFCTL,NACK,SWITCH,MISSING_PARAMS*70` | Brak celu lub stanu, np. `$SFCTL,SWITCH*70`. |
| `$SFCTL,NACK,SWITCH,UNKNOWN_TARGET*71` | Nierozpoznany cel, np. `$SFCTL,SWITCH,INVALID_TARGET,ON*6E`. |
| `$SFCTL,NACK,SWITCH,INVALID_VALUE*32` | Stan inny niż `ON` lub `OFF`, np. `$SFCTL,SWITCH,EXT_LED,BLINK*69`. |

Poprawność celu sprawdzana jest przed poprawnością stanu. Po odpowiedzi negatywnej stan wyjścia pozostaje bez zmian.

### 5.13. EVENT – zdarzenie pilota

**Polecenie:**

```
$SFCTL,EVENT*2E
```

Urządzenie wysyła na port FLARM zdanie zgłaszające zdarzenie pilota:

```
$PFLAI,PILOTEVENT*7C
```

a następnie na port, z którego nadeszło polecenie, potwierdzenie:

```
$SFCTL,ACK,EVENT*4B
```

Ewentualne dodatkowe pola polecenia są ignorowane. Jeżeli FLARM potwierdzi przyjęcie zdarzenia zdaniem `$PFLAI,PILOTEVENT,OK*54`, urządzenie sygnalizuje to sekwencją diody zewnętrznej (świecenie 1000 ms, przerwa 200 ms, świecenie 100 ms, przerwa 100 ms).

---

## 6. Parametry konfiguracyjne

Parametry konfiguracyjne przechowywane są w pamięci nieulotnej. Są dostępne poleceniami `LIST`, `R`, `W` i `DEFAULTS` zdania SFCTL, a także przez interfejs WWW urządzenia. Wszystkie parametry są zapisywalne.

| Nazwa | Typ | Dozwolone wartości | Wartość domyślna | Grupa | Opis |
|---|---|---|---|---|---|
| `DEVICE_NAME` | tekst | dowolny tekst bez znaków zastrzeżonych | `SmartFlasher` | Ogólne | Nazwa urządzenia (m.in. nazwa rozgłaszana przez Bluetooth). |
| `MODE` | wyliczeniowy | `FLARM`, `STANDALONE` | `STANDALONE` | Ogólne | Tryb pracy urządzenia (rozdział 9.3). |
| `POWER_LEVEL` | wyliczeniowy | `20`, `27`, `35`, `40`, `45`, `100` | `35` | Ogólne | Jasność lampy w procentach; odpowiada poziomom 0–5 (rozdział 9.6). |
| `FLARM_BAUD` | wyliczeniowy | `4800`, `9600`, `19200`, `57600`, `115200` | `19200` | Porty | Prędkość transmisji portu FLARM [bit/s]. |
| `FLARM_POWER` | logiczny | `true`, `false` | `false` | Porty | Zasilanie portu FLARM (wraz z żółtą diodą portu). |
| `DISPLAY_BAUD` | wyliczeniowy | `4800`, `9600`, `19200`, `57600`, `115200` | `19200` | Porty | Prędkość transmisji portu DISPLAY [bit/s]. |
| `DISPLAY_POWER` | logiczny | `true`, `false` | `false` | Porty | Zasilanie portu DISPLAY. |
| `BT_MODE` | wyliczeniowy | `OFF`, `CLASSIC`, `BLE` | `OFF` | Łączność | Rodzaj łącza Bluetooth: wyłączone, Bluetooth Classic (SPP), Bluetooth LE (NUS). |
| `BRIDGE_ENABLED` | logiczny | `true`, `false` | `false` | Łączność | Mostek FLARM → DISPLAY. |
| `BRIDGE_TWO_WAY` | logiczny | `true`, `false` | `true` | Łączność | Przekazywanie danych z portów klienckich do FLARM (rozdział 2.3). |
| `OTA_URL` | tekst | adres URL | `https://smartflasher.eu/firmware/manifest` | Zaawansowane | Adres serwera aktualizacji oprogramowania. |
| `DEBUG_FRAMES` | logiczny | `true`, `false` | `false` | Zaawansowane | Parametr serwisowy – włącza wysyłanie diagnostycznych ramek serwisowych. |
| `SFINF_PERIODIC` | logiczny | `true`, `false` | `true` | Zaawansowane | Cykliczne (co 1 s) wysyłanie ramki `$SFINF` (rozdział 8). |

> **Uwaga:** długość wartości parametrów tekstowych jest ograniczona maksymalną długością zdania (rozdział 3.3).

---

## 7. Zdanie SFVAR – zmienne bieżące

### 7.1. Charakterystyka

Zdanie SFVAR służy do odczytu **zmiennych bieżących** – wartości, które nie są parametrami konfiguracyjnymi i mogą się szybko zmieniać (pomiary temperatury i napięcia, czas pracy, stan lotu).

| Cecha | Parametry (SFCTL `R` / `W`) | Zmienne (SFVAR) |
|---|---|---|
| Źródło | pamięć nieulotna | wyznaczane w chwili zapytania |
| Zapis | możliwy | niemożliwy – wyłącznie odczyt |
| Trwałość | zachowywane po ponownym uruchomieniu | nieutrwalane |

### 7.2. Zapytanie (Host → Urządzenie)

```
$SFVAR[,<NAZWA1>[,<NAZWA2>...]]*hh
```

- Wielkość liter w nazwach nie jest rozróżniana.
- W jednym zdaniu można zażądać dowolnej liczby zmiennych, z zachowaniem ograniczenia długości zdania (rozdział 3.3).
- Zapytanie bez nazw oznacza żądanie odczytu wszystkich zmiennych.

### 7.3. Odpowiedź (Urządzenie → Host)

**Zapytanie z nazwami** – jedna ramka zawierająca pary nazwa–wartość w kolejności zgodnej z zapytaniem; nazwy zapisane są wielkimi literami:

```
$SFVAR,<NAZWA1>,<WARTOŚĆ1>[,<NAZWA2>,<WARTOŚĆ2>...]*hh
```

**Zapytanie bez nazw** – dla każdej zmiennej osobna ramka `$SFVAR,<NAZWA>,<WARTOŚĆ>`, w kolejności z tabeli 7.5. Odpowiedź nie jest zakończona ramką potwierdzenia.

**Błędy** – odpowiedź negatywna ma format analogiczny do zdania SFCTL:

```
$SFVAR,NACK,<NAZWA>,<KOD_BŁĘDU>*hh
```

gdzie `<NAZWA>` jest nazwą zmiennej (wielkimi literami), której dotyczy błąd, a dla zapytania bez nazw – słowem `ALL`.

| Ramka | Znaczenie |
|---|---|
| `$SFVAR,NACK,<NAZWA>,UNKNOWN_VAR*hh` | Zmienna o podanej nazwie nie istnieje. Nie są zwracane wartości żadnej ze zmiennych z danego zapytania. |
| `$SFVAR,NACK,<NAZWA>,NOT_AVAILABLE*hh` | Rejestr zmiennych jest niedostępny; `<NAZWA>` to pierwsza zmienna z zapytania lub `ALL`, np. `$SFVAR,NACK,ALL,NOT_AVAILABLE*69`. |

Nazwy `NACK` i `ALL` są zastrzeżone i nie są nazwami zmiennych.

### 7.4. Kierowanie

- Zdanie SFVAR jest obsługiwane na wszystkich portach: FLARM, DISPLAY, Bluetooth Classic, Bluetooth LE i USB.
- Odpowiedź wysyłana jest na port, z którego nadeszło zapytanie.
- Zdanie SFVAR nigdy nie jest przekazywane do FLARM ani mostkowane.

### 7.5. Zmienne

| Nazwa | Jednostka | Format | Opis |
|---|---|---|---|
| `TEMP` | °C | 1 miejsce po przecinku | Temperatura. Wartość `-1000.0` oznacza błąd odczytu. |
| `TEMP_RAW` | – | liczba całkowita 0–4095 | Surowa wartość przetwornika A/C kanału temperatury. |
| `TEMP_MV` | mV | liczba całkowita | Skalibrowane napięcie kanału temperatury. |
| `VOLTAGE` | V | 2 miejsca po przecinku | Napięcie zasilania (z uwzględnieniem dzielnika). |
| `VOLTAGE_RAW` | – | liczba całkowita 0–4095 | Surowa wartość przetwornika A/C kanału napięcia. |
| `VOLTAGE_MV` | mV | liczba całkowita | Skalibrowane napięcie kanału pomiarowego (przed uwzględnieniem dzielnika). |
| `UPTIME` | s | liczba całkowita | Czas od uruchomienia. |
| `HEAP` | B | liczba całkowita | Wolna pamięć operacyjna. |
| `AIRBORNE` | – | 0 / 1 | Stan lotu: 1 – w locie, 0 – na ziemi. |
| `GPS_FIX` | – | 0 / 1 | Ważne położenie GPS raportowane przez FLARM. |
| `SW_VERSION` | – | tekst | Wersja oprogramowania, np. `0.9.106`. |
| `BUILD_TIMESTAMP` | – | tekst `RRRR-MM-DD GG:MM` | Data i godzina kompilacji oprogramowania, np. `2026-05-06 01:28`. |
| `FLARM_CONNECTED` | – | 0 / 1 | Odbierane są dane z urządzenia FLARM. |

Wartości pomiarowe pochodzą z pamięci podręcznej pomiarów, dlatego częste odpytywanie nie wpływa na pracę przetwornika A/C.

### 7.6. Przykłady

| Zapytanie | Odpowiedź |
|---|---|
| `$SFVAR,TEMP*70` | `$SFVAR,TEMP,23.4*47` |
| `$SFVAR,VOLTAGE*3E` | `$SFVAR,VOLTAGE,12.31*3D` |
| `$SFVAR,TEMP,VOLTAGE*1E` | `$SFVAR,TEMP,23.4,VOLTAGE,12.31*2A` |
| `$SFVAR,temp,uptime*4C` | `$SFVAR,TEMP,23.4,UPTIME,3725*54` |
| `$SFVAR,SW_VERSION,BUILD_TIMESTAMP*16` | `$SFVAR,SW_VERSION,0.9.106,BUILD_TIMESTAMP,2026-05-06 01:28*3C` |
| `$SFVAR,FOO*3A` | `$SFVAR,NACK,FOO,UNKNOWN_VAR*6F` |
| `$SFVAR,TEMP,FOO*1A` | `$SFVAR,NACK,FOO,UNKNOWN_VAR*6F` |
| `$SFVAR,foo*1A` | `$SFVAR,NACK,FOO,UNKNOWN_VAR*6F` |

Odczyt wszystkich zmiennych:

```
→ $SFVAR*50
← $SFVAR,TEMP,23.4*47
← $SFVAR,TEMP_RAW,1876*4F
← $SFVAR,TEMP_MV,1520*1E
← $SFVAR,VOLTAGE,12.31*3D
← $SFVAR,VOLTAGE_RAW,2210*08
← $SFVAR,VOLTAGE_MV,1785*5D
← $SFVAR,UPTIME,3725*43
← $SFVAR,HEAP,182344*44
← $SFVAR,AIRBORNE,0*6E
← $SFVAR,GPS_FIX,1*2D
← $SFVAR,SW_VERSION,0.9.106*6F
← $SFVAR,BUILD_TIMESTAMP,2026-05-06 01:28*03
← $SFVAR,FLARM_CONNECTED,1*35
```

---

## 8. Zdanie SFINF – ramka informacyjna

### 8.1. Charakterystyka

Ramka `$SFINF` jest wysyłana samoczynnie przez urządzenie i informuje o jego bieżącym stanie. Przeznaczona jest dla wyświetlaczy i narzędzi testowych.

- Ramka wysyłana jest na port FLARM, a gdy włączony jest mostek – również na porty klienckie.
- Ramka nie jest interpretowana przez urządzenie jako dane wejściowe.

### 8.2. Format

```
$SFINF,<uptime>,<alertState>,<deviceMode>,<flashMode>,<testFlash>,<airborne>,<serverStatus>,<gpsFix>*hh
```

| Nr | Pole | Wartości | Opis |
|---|---|---|---|
| 1 | `uptime` | liczba całkowita | Czas od uruchomienia [s]. |
| 2 | `alertState` | 0–3 | Ogólny poziom alarmu FLARM z pola `alarmLevel` ostatniego zdania `PFLAU` (0 – brak, 1 – niski, 2 – ważny, 3 – pilny). Po utracie danych z FLARM pole przyjmuje wartość 0. |
| 3 | `deviceMode` | 0 / 1 | Tryb pracy: 0 – FLARM, 1 – STANDALONE (zgodnie z parametrem `MODE`). |
| 4 | `flashMode` | 0–4 | Bieżący tryb błysku (0 – brak błysku). |
| 5 | `testFlash` | 0 / 1 | 1 – trwa 30-sekundowa sekwencja testowa. |
| 6 | `airborne` | 0 / 1 | 1 – w locie, 0 – na ziemi. |
| 7 | `serverStatus` | 0 / 1 | 1 – interfejs WWW / tryb ustawień aktywny. |
| 8 | `gpsFix` | 0 / 1 | 1 – ważne położenie GPS. |

### 8.3. Zasady wysyłania

1. Ramka wysyłana jest **natychmiast po zmianie wartości któregokolwiek pola** (z wyjątkiem pola `uptime`).
2. Dodatkowo ramka wysyłana jest **co 1 sekundę**, o ile włączony jest parametr `SFINF_PERIODIC` (ustawienie domyślne). Wyłączenie tego parametru pozostawia wyłącznie wysyłanie przy zmianie stanu.

### 8.4. Przykłady

| Ramka | Znaczenie |
|---|---|
| `$SFINF,112,0,0,1,0,0,1,0*56` | Na ziemi, tryb FLARM, tryb błysku 1, interfejs WWW aktywny, brak testu, brak położenia GPS. |
| `$SFINF,120,0,1,3,0,1,0,1*55` | W locie, tryb STANDALONE, tryb błysku 3, interfejs WWW nieaktywny, brak testu, położenie GPS ważne. |
| `$SFINF,125,0,0,1,1,0,0,1*53` | Trwa sekwencja testowa, tryb FLARM, tryb błysku 1, na ziemi, położenie GPS ważne. |
| `$SFINF,3600,3,0,4,0,1,0,1*66` | W locie, alarm pilny, tryb FLARM, tryb błysku 4, położenie GPS ważne. |

---

## 9. Lampa błyskowa – tryby, sekwencje i poziomy jasności

### 9.1. Tryby błysku

Wszystkie czasy podano w milisekundach. Sekwencje są realizowane cyklicznie.

| Tryb | Wzorzec | Liczba impulsów | Czas cyklu | Częstotliwość | Czas trwania |
|---|---|---|---|---|---|
| 0 | brak błysku | 0 | – | – | – |
| 1 | błysk 40, przerwa 150, błysk 40, przerwa 2000 | 2 | 2230 ms | ok. 0,45 Hz | bez ograniczenia |
| 2 | błysk 50, przerwa 1000 | 1 | 1050 ms | ok. 0,95 Hz | 15 s, następnie tryb 1 |
| 3 | błysk 50, przerwa 650 | 1 | 700 ms | ok. 1,43 Hz | 20 s, następnie tryb 1 |
| 4 | błysk 50, przerwa 300 | 1 | 350 ms | ok. 2,86 Hz | 30 s, następnie tryb 1 |

Czas trwania trybów 2–4 liczony jest od chwili rozpoczęcia danego trybu. Po jego upływie urządzenie samoczynnie przechodzi do trybu 1 (a nie wyłącza lampy). Tryb 1 trwa do czasu zmiany trybu.

### 9.2. Automatyczny dobór trybu na podstawie alarmów FLARM

Gdy lampa jest włączona, urządzenie dobiera tryb błysku na podstawie poziomu alarmu odebranego w zdaniach `PFLAA` i `PFLAU`:

| Poziom alarmu FLARM | Tryb błysku |
|---|---|
| 1 – niski | 2 |
| 2 – ważny | 3 |
| 3 – pilny | 4 |

Tryb jest wyłącznie **podwyższany** – alarm o niższym poziomie nie obniża trybu bieżącego. Powrót do trybu 1 następuje po upływie czasu trwania trybu (tabela 9.1).

### 9.3. Wpływ trybu pracy urządzenia

| Tryb pracy (`MODE`) | Zachowanie lampy |
|---|---|
| `FLARM` | Lampa błyska wyłącznie w locie (rozdział 11.3). Tryb błysku wynika z alarmów FLARM i poleceń `FLASH,MODE`. |
| `STANDALONE` | Lampa błyska zawsze, gdy jest włączona, niezależnie od stanu lotu. Każdy tryb 1–4 jest realizowany jako tryb 1; alarmy FLARM nie zmieniają wzorca. |

### 9.4. Sekwencja startowa

Po włączeniu zasilania lampa wykonuje sekwencję potwierdzającą sprawność lampy i przewodu:

- wzorzec: błysk 200, przerwa 200, powtórzony 5 razy (łącznie 2000 ms),
- po zakończeniu: przerwa 1000 ms, a następnie wznowienie bieżącego trybu.

### 9.5. Sekwencja testowa

Uruchamiana poleceniem `FLASH,TEST` lub z interfejsu WWW:

- wzorzec: błysk 500, przerwa 500, powtarzany przez 30 s (30 błysków),
- sekwencja wstrzymuje bieżący tryb; po jej zakończeniu i przerwie 1000 ms bieżący tryb jest wznawiany (o ile lampa jest włączona),
- w czasie trwania sekwencji pole `testFlash` ramki `$SFINF` ma wartość 1.

### 9.6. Poziomy jasności

Jasność lampy regulowana jest analogowo napięciem sterującym.

| Poziom | Wartość `POWER_LEVEL` | Napięcie sterujące | Nazwa |
|---|---|---|---|
| 0 | `20` | 20 % | minimalna |
| 1 | `27` | 27 % | niska |
| 2 | `35` | 35 % | średnia (domyślna) |
| 3 | `40` | 40 % | wysoka |
| 4 | `45` | 45 % | bardzo wysoka |
| 5 | `100` | 100 % | pełna |

Poziom ustawiony poleceniem `FLASH,BRIGHTNESS` obowiązuje do ponownego uruchomienia lub do zmiany parametru `POWER_LEVEL`. Ustawienie trwałe realizuje parametr `POWER_LEVEL`.

---

## 10. Kody błędów

### 10.1. Kody zdania SFCTL

Odpowiedź negatywna ma format `$SFCTL,NACK,<POLECENIE>,<KOD_BŁĘDU>*hh` (rozdział 5.1).

| Kod | Pole `<POLECENIE>` | Znaczenie | Przykładowa ramka |
|---|---|---|---|
| `UNKNOWN` | odebrana nazwa polecenia | Nierozpoznane polecenie. | `$SFCTL,NACK,PING,UNKNOWN*3D` |
| `UNKNOWN` | `R`, `W` | Nieistniejący parametr. | `$SFCTL,NACK,R,UNKNOWN*7F` |
| `READ_ONLY` | `W` | Parametr tylko do odczytu. | `$SFCTL,NACK,W,READ_ONLY*6B` |
| `INVALID_VALUE` | `W` | Brak nazwy parametru lub wartość niedozwolona dla parametru logicznego albo wyliczeniowego. | `$SFCTL,NACK,W,INVALID_VALUE*77` |
| `INVALID_FLASH_CMD` | `FLASH` | Brak akcji. | `$SFCTL,NACK,FLASH,INVALID_FLASH_CMD*7E` |
| `UNKNOWN_FLASH_ACTION` | `FLASH` | Nierozpoznana akcja. | `$SFCTL,NACK,FLASH,UNKNOWN_FLASH_ACTION*33` |
| `MISSING_MODE` | `FLASH` | Akcja `MODE` bez numeru trybu. | `$SFCTL,NACK,FLASH,MISSING_MODE*2D` |
| `INVALID_MODE` | `FLASH` | Tryb spoza zakresu 0–4. | `$SFCTL,NACK,FLASH,INVALID_MODE*38` |
| `MISSING_BRIGHTNESS` | `FLASH` | Akcja `BRIGHTNESS` bez poziomu jasności. | `$SFCTL,NACK,FLASH,MISSING_BRIGHTNESS*27` |
| `INVALID_BRIGHTNESS` | `FLASH` | Poziom spoza zakresu 0–5. | `$SFCTL,NACK,FLASH,INVALID_BRIGHTNESS*32` |
| `TEST_ALREADY_ACTIVE` | `FLASH` | Sekwencja testowa jest już w toku. | `$SFCTL,NACK,FLASH,TEST_ALREADY_ACTIVE*69` |
| `MISSING_PARAMS` | `SWITCH` | Brak celu lub stanu. | `$SFCTL,NACK,SWITCH,MISSING_PARAMS*70` |
| `UNKNOWN_TARGET` | `SWITCH` | Nierozpoznany cel. | `$SFCTL,NACK,SWITCH,UNKNOWN_TARGET*71` |
| `INVALID_VALUE` | `SWITCH` | Stan inny niż `ON`/`OFF`. | `$SFCTL,NACK,SWITCH,INVALID_VALUE*32` |

Kody `TIMEOUT`, `BUSY`, `PARSE_ERROR` i `ACCESS_DENIED` są zarezerwowane do wykorzystania w przyszłych wersjach protokołu. Host powinien traktować każdy nieznany kod błędu jako odrzucenie polecenia.

### 10.2. Kody zdania SFVAR

Odpowiedź negatywna ma format `$SFVAR,NACK,<NAZWA>,<KOD_BŁĘDU>*hh` (rozdział 7.3).

| Kod | Pole `<NAZWA>` | Znaczenie | Przykładowa ramka |
|---|---|---|---|
| `UNKNOWN_VAR` | nazwa nieistniejącej zmiennej | Nieistniejąca zmienna. | `$SFVAR,NACK,FOO,UNKNOWN_VAR*6F` |
| `NOT_AVAILABLE` | pierwsza zmienna z zapytania lub `ALL` | Rejestr zmiennych niedostępny. | `$SFVAR,NACK,ALL,NOT_AVAILABLE*69` |

### 10.3. Postępowanie po odebraniu NACK

1. Na podstawie drugiego pola ramki (`<POLECENIE>` lub `<NAZWA>`) ustalić, którego żądania dotyczy odpowiedź.
2. Zweryfikować składnię polecenia i wartości argumentów.
3. W przypadku kodu `UNKNOWN` sprawdzić nazwę polecenia lub parametru (wielkość liter, lista z polecenia `LIST`).
4. Ponowić polecenie po usunięciu przyczyny błędu.

---

## 11. Zdania wejściowe FLARM i GPS

### 11.1. Zdania rozpoznawane

Na portach pełnych urządzenie rozpoznaje poniższe zdania. Są one zliczane w statystykach (`STATS`), mostkowane zgodnie z rozdziałem 2.4, a wybrane z nich wpływają na pracę urządzenia.

| Zdanie | Opis | Wpływ na pracę urządzenia |
|---|---|---|
| `PFLAA` | Dane o pojedynczym obiekcie w otoczeniu | Poziom alarmu podwyższa tryb błysku (rozdział 9.2). |
| `PFLAU` | Stan pracy FLARM | Sygnalizuje połączenie z FLARM (`FLARM_CONNECTED`), aktualizuje stan GPS (`GPS_FIX`) i pole `alertState` ramki `$SFINF`; poziom alarmu podwyższa tryb błysku. |
| `PFLAC` | Konfiguracja FLARM | Brak – wyłącznie zliczanie i mostkowanie. |
| `PFLAE` | Błędy i ostrzeżenia FLARM | Brak – wyłącznie zliczanie i mostkowanie. |
| `PFLAI` | Odpowiedzi FLARM | Potwierdzenie `PILOTEVENT,OK` sygnalizowane diodą zewnętrzną (rozdział 5.13). |
| `GPGGA` | Dane położenia GPS | Aktualizuje stan GPS (`GPS_FIX`). |
| `GPRMC` | Minimalne dane nawigacyjne | Na podstawie prędkości wyznacza stan lotu (rozdział 11.3). |
| `GPGSA` | Aktywne satelity, współczynniki DOP | Brak – wyłącznie zliczanie i mostkowanie. |
| `GPGSV` | Satelity w zasięgu | Brak – wyłącznie zliczanie i mostkowanie. |
| `GPGLL` | Położenie geograficzne | Brak – wyłącznie zliczanie i mostkowanie. |

### 11.2. Opis pól i przykłady

#### PFLAA – dane o obiekcie (jedno zdanie na obiekt)

Kolejność pól: `alarmLevel, relN, relE, relV, idType, id, track, turnRate, groundSpeed, climbRate, acftType`

| Pole | Wartości |
|---|---|
| `alarmLevel` | 0 – brak, 1 – niski (13–18 s do kolizji), 2 – ważny (8–12 s), 3 – pilny (0–7 s) |
| `relN`, `relE`, `relV` | odległość względna [m]; wartości dodatnie: północ / wschód / powyżej |
| `idType` | 0 – losowy, 1 – ICAO, 2 – FLARM, 3 – OGN |
| `id` | identyfikator szesnastkowy |
| `track` | kurs [°], 0–359 |
| `turnRate` | prędkość kątowa zakrętu [°/s] |
| `groundSpeed` | prędkość względem ziemi [m/s] |
| `climbRate` | prędkość pionowa [m/s], wartość dodatnia – wznoszenie |
| `acftType` | typ statku powietrznego (szesnastkowo), np. 1 – szybowiec, 8 – samolot z napędem |

```
$PFLAA,0,1234,-500,120,2,A1B2C3,270,0,15,2,1*29
$PFLAA,1,800,-200,50,1,4B1A35,180,0,30,0,8*56
$PFLAA,2,300,100,-80,2,DE1234,90,5,40,-3,1*43
$PFLAA,3,150,50,0,2,FF0011,0,0,0,0,1*47
```

Kolejno: brak alarmu; alarm niski; alarm ważny; alarm pilny.

#### PFLAU – stan pracy FLARM (raz na cykl)

Kolejność pól: `rx, tx, gps, power, alarmLevel, relativeBearing, alarmType, relV, relDist[, id]`

| Pole | Wartości |
|---|---|
| `rx` | liczba odbieranych urządzeń FLARM |
| `tx` | 0 – brak nadawania, 1 – nadawanie |
| `gps` | 0 – brak położenia, 1 – położenie 3D, 2 – położenie 3D + DGPS |
| `power` | 0 – zasilanie prawidłowe, 1 – alarm zasilania |
| `alarmLevel` | 0–3, jak w `PFLAA` |
| `relativeBearing` | namiar względny na najgroźniejszy obiekt [°], −180…+180 |
| `alarmType` | 0 – statek powietrzny, 2 – przeszkoda, 3 – strefa |
| `relV` | separacja pionowa od najgroźniejszego obiektu [m] |
| `relDist` | odległość pozioma od najgroźniejszego obiektu [m] |
| `id` | (opcjonalnie) identyfikator najgroźniejszego obiektu |

```
$PFLAU,3,1,2,0,0,0,0,0,0*52
$PFLAU,1,1,2,0,1,45,0,120,800,A1B2C3*37
$PFLAU,2,1,1,0,2,-30,0,-50,400,4B1A35*7C
$PFLAU,0,0,0,0,0,0,0,0,0*52
```

Kolejno: 3 urządzenia, brak alarmu; alarm niski; alarm ważny; brak urządzeń i brak GPS.

#### PFLAC – konfiguracja FLARM

Kolejność pól: `polecenie[, parametr1[, parametr2]]`

```
$PFLAC,S,ID,DE1234*2F
$PFLAC,A,NMEAOUT,2*4E
$PFLAC,Q,ID*04
```

#### PFLAE – błędy i ostrzeżenia FLARM

Kolejność pól: `severity, errorCode, errorMsg` (`severity`: `A` – ostrzeżenie, `E` – błąd)

```
$PFLAE,A,21,Low battery*29
$PFLAE,E,11,GPS signal lost*69
$PFLAE,A,0,No error*7A
```

#### PFLAI – zdarzenie pilota

```
$PFLAI,PILOTEVENT*7C
$PFLAI,PILOTEVENT,OK*54
```

Kolejno: zgłoszenie zdarzenia (wysyłane przez urządzenie); potwierdzenie przez FLARM.

#### GPGGA – dane położenia

Kolejność pól: `time, lat, latDir, lon, lonDir, quality, numSats, hdop, alt, M, geoid, M, , ` (`quality`: 0 – brak położenia, 1 – GPS, 2 – DGPS)

```
$GPGGA,123519.00,4807.0382,N,01131.0000,E,1,08,0.9,545.4,M,46.9,M,,*6B
$GPGGA,000000.00,0000.0000,N,00000.0000,E,0,00,99.9,0.0,M,0.0,M,,*64
```

#### GPRMC – minimalne dane nawigacyjne

Kolejność pól: `time, status, lat, latDir, lon, lonDir, speedKnots, course, date, , ` (`status`: `A` – dane ważne, `V` – dane nieważne)

```
$GPRMC,123519.00,A,4807.0382,N,01131.0000,E,0.0,360.0,230394,,*34
$GPRMC,123520.00,A,4807.1020,N,01131.0400,E,45.0,005.0,230394,,*01
$GPRMC,000000.00,V,0000.0000,N,00000.0000,E,0.0,0.0,010124,,*22
```

Kolejno: dane ważne, prędkość 0; dane ważne, prędkość 45 węzłów (ok. 83 km/h – stan lotu); dane nieważne.

#### GPGSA – aktywne satelity i współczynniki DOP

Kolejność pól: `mode1, mode2, prn×12, pdop, hdop, vdop` (`mode1`: `A` – automatyczny, `M` – ręczny; `mode2`: 1 – brak, 2 – 2D, 3 – 3D)

```
$GPGSA,A,3,01,02,03,04,05,06,07,08,00,00,00,00,1.8,1.0,1.5*36
$GPGSA,A,1,00,00,00,00,00,00,00,00,00,00,00,00,99.9,99.9,99.9*09
```

#### GPGSV – satelity w zasięgu

Kolejność pól: `totalMsgs, msgNum, totalSats, [prn, elev, az, snr] × maks. 4`

```
$GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75
$GPGSV,2,2,08,15,51,136,44,17,16,059,40,19,08,301,38,21,30,175,47*75
```

#### GPGLL – położenie geograficzne

Kolejność pól: `lat, latDir, lon, lonDir, time, status, modeIndicator`

```
$GPGLL,4807.0382,N,01131.0000,E,123519.00,A,A*64
$GPGLL,0000.0000,N,00000.0000,E,000000.00,V,N*71
```

### 11.3. Wyznaczanie stanu lotu

Stan lotu (`AIRBORNE`, pole `airborne` ramki `$SFINF`) wyznaczany jest na podstawie prędkości ze zdań `GPRMC` o statusie `A`:

1. Prędkość powyżej **30 km/h** – urządzenie przechodzi w stan „w locie”; trwające odliczanie lądowania zostaje anulowane.
2. Prędkość nie większa niż **5 km/h** w stanie „w locie” – rozpoczyna się odliczanie lądowania.
3. Jeżeli przez **3 minuty** od rozpoczęcia odliczania prędkość nie przekroczy ponownie 30 km/h, urządzenie przechodzi w stan „na ziemi”.

### 11.4. Stan położenia GPS

Pole `gpsFix` ramki `$SFINF` oraz zmienna `GPS_FIX` przyjmują wartość 1, gdy:

- zdanie `PFLAU` zawiera w polu `gps` wartość 1 lub 2, lub
- zdanie `GPGGA` zawiera ważne położenie.

Wartość 0 ustawiana jest, gdy `PFLAU` raportuje `gps` = 0 lub `GPGGA` nie zawiera ważnego położenia.

---

## 12. Przykładowe sesje komunikacyjne

### 12.1. Odczyt, zapis i weryfikacja parametru

```
→ $SFCTL,R,POWER_LEVEL*4A
← $SFCTL,ACK,POWER_LEVEL,35*7B
→ $SFCTL,W,POWER_LEVEL,45*62
← $SFCTL,ACK,POWER_LEVEL,45*7C
→ $SFCTL,R,POWER_LEVEL*4A
← $SFCTL,ACK,POWER_LEVEL,45*7C
```

### 12.2. Obsługa błędów

```
→ $SFCTL,R,INVALID_PARAM*5D
← $SFCTL,NACK,R,UNKNOWN*7F
→ $SFCTL,list*60
← $SFCTL,NACK,list,UNKNOWN*2F
→ $SFCTL,FLASH,MODE,7*06
← $SFCTL,NACK,FLASH,INVALID_MODE*38
→ $SFCTL,FLASH,BLINK*5C
← $SFCTL,NACK,FLASH,UNKNOWN_FLASH_ACTION*33
→ $SFCTL,W,BT_MODE,WIFI*6E
← $SFCTL,NACK,W,INVALID_VALUE*77
→ $SFCTL,W,FLARM_POWER,yes*0E
← $SFCTL,NACK,W,INVALID_VALUE*77
→ $SFCTL,SWITCH,POWER_LED,TOGGLE*2F
← $SFCTL,NACK,SWITCH,INVALID_VALUE*32
```

### 12.3. Konfiguracja i kontrola lampy

```
→ $SFCTL,FLASH,BRIGHTNESS,3*08
← $SFCTL,ACK,FLASH_BRIGHTNESS,3*1E
→ $SFCTL,FLASH,MODE,2*03
← $SFCTL,ACK,FLASH_MODE,2*15
→ $SFCTL,FLASH,ON*1F
← $SFCTL,ACK,FLASH,ON*7A
→ $SFCTL,FLASH,TEST*08
← $SFCTL,ACK,FLASH_TEST,STARTED*73
→ $SFCTL,FLASH,TEST*08
← $SFCTL,NACK,FLASH,TEST_ALREADY_ACTIVE*69
→ $SFCTL,FLASH,STATUS*0A
← $SFCTL,STATUS,FLASH_ON,1*49
← $SFCTL,STATUS,FLASH_MODE,2*48
← $SFCTL,STATUS,FLASH_BRIGHTNESS,3*43
← $SFCTL,STATUS,FLASH_TEST,1*5E
← $SFCTL,ACK,FLASH_STATUS,COMPLETE*39
→ $SFCTL,FLASH,OFF*51
← $SFCTL,ACK,FLASH,OFF*34
```

### 12.4. Włączenie mostka do wyświetlacza

```
→ $SFCTL,W,DISPLAY_POWER,true*79
← $SFCTL,ACK,DISPLAY_POWER,true*67
→ $SFCTL,W,BRIDGE_ENABLED,true*26
← $SFCTL,ACK,BRIDGE_ENABLED,true*38
→ $SFCTL,W,BRIDGE_TWO_WAY,true*3F
← $SFCTL,ACK,BRIDGE_TWO_WAY,true*21
```

### 12.5. Zdarzenie pilota

```
→ $SFCTL,EVENT*2E                       (port DISPLAY)
  $PFLAI,PILOTEVENT*7C                  (urządzenie → FLARM)
← $SFCTL,ACK,EVENT*4B                   (port DISPLAY)
  $PFLAI,PILOTEVENT,OK*54               (FLARM → urządzenie; potwierdzenie diodą zewnętrzną)
```

### 12.6. Ponowne uruchomienie

```
→ $SFCTL,RESET*37
← $SFCTL,ACK,RESET,RESTARTING*69
  (po ok. 100 ms urządzenie uruchamia się ponownie; lampa wykonuje sekwencję startową)
```

---

## 13. Załącznik A – zestawienie ramek

### 13.1. Polecenia bez argumentów

| Polecenie | Ramka |
|---|---|
| Lista parametrów | `$SFCTL,LIST*60` |
| Ustawienia fabryczne | `$SFCTL,DEFAULTS*7A` |
| Stan systemu | `$SFCTL,STATUS*76` |
| Statystyki | `$SFCTL,STATS*23` |
| Ponowne uruchomienie | `$SFCTL,RESET*37` |
| Informacje systemowe | `$SFCTL,SYSINFO*35` |
| Zdarzenie pilota | `$SFCTL,EVENT*2E` |
| Wszystkie zmienne | `$SFVAR*50` |

### 13.2. Odczyt parametrów

| Parametr | Ramka |
|---|---|
| `DEVICE_NAME` | `$SFCTL,R,DEVICE_NAME*5C` |
| `MODE` | `$SFCTL,R,MODE*1F` |
| `POWER_LEVEL` | `$SFCTL,R,POWER_LEVEL*4A` |
| `FLARM_BAUD` | `$SFCTL,R,FLARM_BAUD*05` |
| `FLARM_POWER` | `$SFCTL,R,FLARM_POWER*48` |
| `DISPLAY_BAUD` | `$SFCTL,R,DISPLAY_BAUD*0B` |
| `DISPLAY_POWER` | `$SFCTL,R,DISPLAY_POWER*46` |
| `BT_MODE` | `$SFCTL,R,BT_MODE*56` |
| `BRIDGE_ENABLED` | `$SFCTL,R,BRIDGE_ENABLED*19` |
| `BRIDGE_TWO_WAY` | `$SFCTL,R,BRIDGE_TWO_WAY*00` |
| `OTA_URL` | `$SFCTL,R,OTA_URL*52` |
| `DEBUG_FRAMES` | `$SFCTL,R,DEBUG_FRAMES*1C` |
| `SFINF_PERIODIC` | `$SFCTL,R,SFINF_PERIODIC*18` |

### 13.3. Zapis parametrów (przykłady)

| Operacja | Ramka | Odpowiedź |
|---|---|---|
| Tryb pracy FLARM | `$SFCTL,W,MODE,FLARM*62` | `$SFCTL,ACK,MODE,FLARM*7C` |
| Tryb pracy STANDALONE | `$SFCTL,W,MODE,STANDALONE*33` | `$SFCTL,ACK,MODE,STANDALONE*2D` |
| Jasność 100 % | `$SFCTL,W,POWER_LEVEL,100*52` | `$SFCTL,ACK,POWER_LEVEL,100*4C` |
| Port FLARM 57600 bit/s | `$SFCTL,W,FLARM_BAUD,57600*18` | `$SFCTL,ACK,FLARM_BAUD,57600*06` |
| Port DISPLAY 9600 bit/s | `$SFCTL,W,DISPLAY_BAUD,9600*2D` | `$SFCTL,ACK,DISPLAY_BAUD,9600*33` |
| Zasilanie portu FLARM | `$SFCTL,W,FLARM_POWER,true*77` | `$SFCTL,ACK,FLARM_POWER,true*69` |
| Bluetooth Classic | `$SFCTL,W,BT_MODE,CLASSIC*3B` | `$SFCTL,ACK,BT_MODE,CLASSIC*25` |
| Bluetooth LE | `$SFCTL,W,BT_MODE,BLE*34` | `$SFCTL,ACK,BT_MODE,BLE*2A` |
| Wyłączenie cyklicznej ramki SFINF | `$SFCTL,W,SFINF_PERIODIC,false*4C` | `$SFCTL,ACK,SFINF_PERIODIC,false*52` |
| Zmiana nazwy urządzenia | `$SFCTL,W,DEVICE_NAME,Glider-SP3456*6E` | `$SFCTL,ACK,DEVICE_NAME,Glider-SP3456*70` |

### 13.4. Sterowanie lampą

| Operacja | Ramka | Odpowiedź |
|---|---|---|
| Włączenie | `$SFCTL,FLASH,ON*1F` | `$SFCTL,ACK,FLASH,ON*7A` |
| Wyłączenie | `$SFCTL,FLASH,OFF*51` | `$SFCTL,ACK,FLASH,OFF*34` |
| Tryb 0 | `$SFCTL,FLASH,MODE,0*01` | `$SFCTL,ACK,FLASH_MODE,0*17` |
| Tryb 1 | `$SFCTL,FLASH,MODE,1*00` | `$SFCTL,ACK,FLASH_MODE,1*16` |
| Tryb 2 | `$SFCTL,FLASH,MODE,2*03` | `$SFCTL,ACK,FLASH_MODE,2*15` |
| Tryb 3 | `$SFCTL,FLASH,MODE,3*02` | `$SFCTL,ACK,FLASH_MODE,3*14` |
| Tryb 4 | `$SFCTL,FLASH,MODE,4*05` | `$SFCTL,ACK,FLASH_MODE,4*13` |
| Jasność 0 | `$SFCTL,FLASH,BRIGHTNESS,0*0B` | `$SFCTL,ACK,FLASH_BRIGHTNESS,0*1D` |
| Jasność 1 | `$SFCTL,FLASH,BRIGHTNESS,1*0A` | `$SFCTL,ACK,FLASH_BRIGHTNESS,1*1C` |
| Jasność 2 | `$SFCTL,FLASH,BRIGHTNESS,2*09` | `$SFCTL,ACK,FLASH_BRIGHTNESS,2*1F` |
| Jasność 3 | `$SFCTL,FLASH,BRIGHTNESS,3*08` | `$SFCTL,ACK,FLASH_BRIGHTNESS,3*1E` |
| Jasność 4 | `$SFCTL,FLASH,BRIGHTNESS,4*0F` | `$SFCTL,ACK,FLASH_BRIGHTNESS,4*19` |
| Jasność 5 | `$SFCTL,FLASH,BRIGHTNESS,5*0E` | `$SFCTL,ACK,FLASH_BRIGHTNESS,5*18` |
| Test | `$SFCTL,FLASH,TEST*08` | `$SFCTL,ACK,FLASH_TEST,STARTED*73` |
| Stan | `$SFCTL,FLASH,STATUS*0A` | wiersze `STATUS` + `$SFCTL,ACK,FLASH_STATUS,COMPLETE*39` |

### 13.5. Odczyt zmiennych

| Zmienna | Zapytanie |
|---|---|
| `TEMP` | `$SFVAR,TEMP*70` |
| `TEMP_RAW` | `$SFVAR,TEMP_RAW*6B` |
| `TEMP_MV` | `$SFVAR,TEMP_MV*34` |
| `VOLTAGE` | `$SFVAR,VOLTAGE*3E` |
| `VOLTAGE_RAW` | `$SFVAR,VOLTAGE_RAW*25` |
| `VOLTAGE_MV` | `$SFVAR,VOLTAGE_MV*7A` |
| `UPTIME` | `$SFVAR,UPTIME*6C` |
| `HEAP` | `$SFVAR,HEAP*60` |
| `AIRBORNE` | `$SFVAR,AIRBORNE*72` |
| `GPS_FIX` | `$SFVAR,GPS_FIX*30` |
| `SW_VERSION` | `$SFVAR,SW_VERSION*7D` |
| `BUILD_TIMESTAMP` | `$SFVAR,BUILD_TIMESTAMP*3B` |
| `FLARM_CONNECTED` | `$SFVAR,FLARM_CONNECTED*28` |

---

## 14. Historia zmian dokumentu

| Wersja | Data | Opis zmian |
|---|---|---|
| 1.0 | 2026-11-14 | Pierwsze wydanie ujednoliconej specyfikacji protokołu (SFCTL, SFVAR, SFINF, zdania wejściowe FLARM/GPS). |
| 1.1 | 2026-11-14 | Kod `INVALID_VALUE` dla poleceń `W` (brak nazwy, wartość spoza listy, niepoprawna wartość logiczna) i `SWITCH` (niepoprawny stan); pole `alertState` ramki `$SFINF` ustawiane na podstawie `PFLAU`. |
| 1.2 | 2026-11-14 | Zmiana niekompatybilna: ujednolicony format odpowiedzi negatywnych `$SFCTL,NACK,<POLECENIE>,<KOD_BŁĘDU>` oraz `$SFVAR,NACK,<NAZWA>,<KOD_BŁĘDU>`. |

---

*Koniec dokumentu*
