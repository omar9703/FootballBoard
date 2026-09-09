# Tablero táctico de fútbol

Proyecto C++ para Unreal Engine 5.8 basado en el tutorial v5.

## Abrir y usar

Abre `FootballBoard.uproject`. El nivel inicial es `/Game/FootballBoard/Level_Sender`. Pulsa Play y arrastra los círculos rojos o azules con el botón izquierdo o con un dedo en una pantalla táctil. El sender solo muestra círculos, desde arriba.

El receiver muestra únicamente a Manny y Quinn sobre la cancha, sin bases, con cámara isométrica y escala de 2.3. En su BoardManager puedes ajustar `Receiver Player Scale`. Usan `MF_Unarmed_Walk_Fwd` al moverse y `MM_Idle` al detenerse, tolerando pausas breves de red para evitar reinicios de la caminata. Puedes cambiar `Player Walk Anim` y `Player Idle Anim`. El receiver sigue por TCP las posiciones del sender; no hay navegación autónoma.

Para abrir ambas vistas sin dos editores, ejecuta `IniciarTableros.ps1` desde PowerShell. Abre dos ventanas de juego, limita a 60 FPS y mantiene activa la ventana en segundo plano. Usa el motor instalado en `F:\unreal\UE_5.8`. Puedes indicar otra ruta mediante `-UnrealEditor`.

La comunicación usa TCP en el puerto 7777. En esta máquina el receptor utiliza `127.0.0.1`. Para dos equipos, cambia `Server IP` en el BoardManager de Level_Receiver por la dirección del emisor y guarda el nivel. Solo debe haber un Sender escuchando en el mismo puerto.

## Ajustes tactiles del Sender

En BoardManager, categoria Touch, `Touch Pick Padding` amplia el area de seleccion sin cambiar el tamano de las fichas. Su valor inicial es 24 pixeles adicionales a una altura de ventana de referencia de 1080 pixeles; se adapta a la altura real de la ventana.

`Touch Release Grace Seconds` conserva el arrastre durante cortes de contacto de hasta 0.15 segundos, dejando la ficha quieta mientras no hay contacto. Para reanudar, el dedo debe regresar cerca del ultimo punto: `Touch Resume Distance` vale 80 pixeles a la misma resolucion de referencia. Un toque lejano inicia otra seleccion. Se admite una ficha con el primer contacto tactil; no es arrastre multitactil. El movimiento se calcula sobre un plano fijo de la cancha.

Para comprobarlo en la pantalla real: tocar justo fuera de una ficha, arrastrarla sobre otras fichas, interrumpir brevemente el contacto y retomarlo cerca, y soltar antes de tocar otra ficha distante. Verificar tambien que el raton siga soltando inmediatamente. Ajustar los valores segun el hardware; una tolerancia mayor tambien puede interpretar dos toques cercanos como un solo arrastre.

## Archivos del proyecto

- `Source/FootballBoard`: campo, piezas y comunicación C++.
- `Content/FootballBoard`: niveles Sender y Receiver.
- `Scripts/setup_board.py`: creación inicial de escenas; protege niveles ya configurados y no debe ejecutarse de nuevo sobre ellos.

El césped y las fichas se generan al pulsar Play, por lo que el nivel en edición contiene principalmente luces y BoardManager.
