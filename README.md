# Doctor Migrana - Noise Gate

Por **GuitarFros**.

Plugin de pedal de guitarra tipo **noise gate**, en **VST3** e **Standalone** (Linux, Windows e macOS), feito con JUCE 8 e CMake.

A interface é unha imaxe dun pedal artesanal: un só knob (**Threshold**), o footswitch (activa/desactiva) e o LED. A imaxe do pedal está **xerada con IA** (ver [Arte e imaxes](#arte-e-imaxes)).

## Compilar en Ubuntu

```bash
./build.sh
```

O script instala as dependencias (pide `sudo`), descarga JUCE, compila en Release, corre os tests do DSP e copia o VST3 a `~/.vst3`.

Opcións: `--no-deps` (non instala paquetes), `--clean` (borra `build/`), `--no-install` (non copia a `~/.vst3`).

Resultados en `build/DoctorMigranaNoiseGate_artefacts/Release/`:

- `VST3/Doctor Migrana - Noise Gate.vst3`
- `Standalone/Doctor Migrana - Noise Gate` (executable)

A primeira vez tarda un pouco (JUCE é grande). O nome leva espazos: cita sempre a ruta entre aspas na terminal.

### Standalone: ollo coa realimentación
Se usas altofalantes, o standalone pode realimentar. Usa auriculares ou a interface de audio, e escolle o dispositivo desde o botón de opcións da xanela.

## Como se usa

| Control | Que fai |
|---|---|
| **Knob THRESHOLD** | Nivel por debaixo do cal o gate empeza a pechar (−80 a −10 dB). Arrastra en vertical; dobre clic = valor por defecto (−55 dB). |
| **Footswitch** | Activa / desactiva o gate. O LED apágase en bypass. |
| **LED** | Brillo pleno co gate aberto, medio co gate pechado, apagado en bypass. |
| **Dobre clic na placa de latón** | Abre/pecha os axustes avanzados: **Decay** (10–1500 ms), **Hold** (0–200 ms) e **Depth** (6–80 dB; 80 = case mute). |

Todos os parámetros tamén están dispoñibles para automatización no teu DAW.

O plugin introduce **1 ms de latencia** (lookahead) para non comer o ataque da púa; o DAW compénsao automaticamente.

## Cambiar a imaxe

Substitúe `assets/pedal.jpg` e volve compilar. Se a nova imaxe ten outras proporcións ou os elementos están noutro sitio, axusta só `Source/PedalLayout.h` (coordenadas normalizadas de knob, LED, footswitch e placa).

## Estrutura

```
Source/NoiseGate.h        DSP (C++ puro, sen JUCE)
Source/PluginProcessor.*  parámetros e procesado
Source/PluginEditor.*     interface sobre a imaxe
Source/PedalLayout.h      posicións dos elementos
tests/dsp_test.cpp        tests do DSP (ctest)
assets/pedal.jpg          imaxe do pedal (xerada con IA, ver assets/README.md)
NOTICE.md                 avisos de terceiros e da arte
.github/workflows/        compilación Linux/Windows/macOS
```

## Como funciona o gate

Expander descendente con detector de pico, joelho suave, hold e release (decay) axustables, e un pequeno lookahead. A idea é a que perseguen pedais como o Boss NS-2: deixar pasar o ataque e a cola natural da nota e quitar só o ruído. O algoritmo é propio; non copia ningún circuíto nin tecnoloxía patentada.

## GitHub (Linux + Windows + macOS)

Sube o proxecto a GitHub e o workflow `.github/workflows/build.yml` compila as tres plataformas en cada push. Os binarios saen como *artifacts* do job (pestana **Actions**).

Para publicar unha **Release** cos tres `.zip`, crea e sube unha etiqueta que empece por `v`:

```bash
git tag v1.0.0
git push origin v1.0.0
```

A Release só se crea se **os tres builds (Linux, Windows e macOS) acaban en verde**. Os ficheiros chámanse `Doctor-Migrana-Noise-Gate-Linux.zip`, `...-Windows.zip` e `...-macOS.zip`. Un push normal a `main` non crea Release: só a etiqueta `v*`.

Nota: en macOS os binarios non van asinados, e para probalos pode facer falta `xattr -dr com.apple.quarantine "Doctor Migrana - Noise Gate.vst3"`.

## Licenza

Este proxecto é software libre baixo **GNU AGPL v3** (ver `LICENSE`). Podes usalo, compartilo e modificalo, e tamén repartir os binarios, sempre que manteñas a mesma licenza e ofrezas o código fonte (por exemplo, enlazando este repositorio).

- **JUCE 8** está dobremente licenciado (AGPLv3 ou licenza comercial de JUCE). Este proxecto usa JUCE baixo a **AGPLv3**, por iso o resultado tamén é AGPLv3. Non fai falta ningunha opción especial de compilación.
- **A arte** (`assets/`) **non** está baixo a AGPL: ten as condicións propias descritas en [Arte e imaxes](#arte-e-imaxes).
- O plugin VST3 inclúe o SDK de VST3 que trae JUCE. Ver `NOTICE.md`.

Isto é información práctica, non asesoramento legal.

## Arte e imaxes

- **Imaxe do pedal (`assets/pedal.jpg`):** foi **xerada con Meta AI** e despois retocada polo autor (retirouse un texto da placa e reescalouse). O autor do proxecto non reclama dereitos de autor sobre a parte xerada pola IA, e o seu uso queda suxeito aos termos de Meta aplicables. Non está baixo a AGPL.
- **Logo «Doctor Migraña»** (o que aparece na placa de latón): propiedade do autor do proxecto (GuitarFros). **Todos os dereitos reservados**; non está baixo a AGPL.
- **Knob, LED, footswitch e panel de axustes:** debúxanse por código e si están baixo a AGPL.

Se fas unha versión modificada, substitúe a imaxe e o logo pola túa propia arte (ver `assets/README.md`). Sen `assets/pedal.jpg` o plugin compila igual, cun fondo liso.

## Marcas

«Boss», «NS-2», «ISP» e «Decimator» son marcas dos seus propietarios. Este plugin non ten relación con elas.
