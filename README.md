# Panceta Levels List – mod Geode

Mod do Geometry Dash (Geode v5, GD 2.2081), który pokazuje w grze listę PLL i GPLL.

## Co robi
- Przycisk **PLL** (zielone kółko) w menu głównym i na stronie Create.
- Okno z zakładkami **PLL** / **GPLL**.
- PLL: pobiera na żywo wszystkie levele z `https://pll-plus.levelslist.workers.dev/api/levels`
  (to ten sam endpoint, z którego korzysta strona /list), pogrupowane wg kategorii
  (Hardest, High, Mid, Low, Extended, UPLL only), z punktami, List%, liczbą victorów.
- **View** otwiera level w wyszukiwarce GD, ikona YT otwiera film.
- Lista jest **na żywo**: przy każdym otwarciu pobiera świeże dane, a gdy okno jest otwarte, odświeża się sama co 60 s (ustawienie `refresh-interval`). Nie skacze przy tym na górę listy.
- Kliknięcie **"X victors >"** pokazuje listę victorów levela (z `/api/levels/{id}`): nick, kraj, verifier, link do filmu. Ona też odświeża się sama.
- GPLL: na razie "coming soon".

## Jak dodać GPLL później
Najprościej: jeśli GPLL będzie mieć API w tym samym formacie co PLL (`{ "levels": [...], "categories": [...] }`),
wpisz jego bazowy adres API (np. `https://.../api`) jako domyślną wartość `gpll-api-base` w `mod.json` (albo w ustawieniach moda w grze) – nic więcej nie trzeba zmieniać.

## Budowanie
**Najłatwiej przez GitHub:** wrzuć ten folder do nowego repo na GitHubie. Workflow w `.github/workflows`
sam zbuduje mod dla Windows, macOS i Androida – gotowy plik `.geode` pobierzesz z zakładki
*Actions → ostatni build → Artifacts*. Wrzuć go do folderu `geode/mods` w GD.

**Lokalnie:** zainstaluj Geode SDK + CLI (https://docs.geode-sdk.org/getting-started/), potem:
```
geode build
```

Przed publikacją zmień w `mod.json` pola `id` i `developer` (teraz są tam zastępcze `yourname` / `YourName`).
