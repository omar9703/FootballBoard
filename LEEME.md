# Tablero táctico de fútbol

Proyecto C++ para Unreal Engine 5.8 basado en el tutorial v5.

## Abrir y usar

Abre `FootballBoard.uproject`. El nivel inicial es `/Game/FootballBoard/Level_Sender`. Pulsa Play y arrastra los círculos rojos o azules con el botón izquierdo o con un dedo en una pantalla táctil. El sender solo muestra círculos, desde arriba.

El receiver muestra únicamente a Manny y Quinn sobre la cancha, sin bases, con cámara isométrica y escala de 2.3. En su BoardManager puedes ajustar `Receiver Player Scale`. Usan `MF_Unarmed_Walk_Fwd` al moverse y `MM_Idle` al detenerse, tolerando pausas breves de red para evitar reinicios de la caminata. Puedes cambiar `Player Walk Anim` y `Player Idle Anim`. El receiver sigue por TCP las posiciones del sender; no hay navegación autónoma.

Para abrir ambas vistas sin dos editores, ejecuta `IniciarTableros.ps1` desde PowerShell. Abre dos ventanas de juego, limita a 60 FPS y mantiene activa la ventana en segundo plano. Usa el motor instalado en `F:\unreal\UE_5.8`. Puedes indicar otra ruta mediante `-UnrealEditor`.

La comunicación usa TCP en el puerto 7777. En esta máquina el receptor utiliza `127.0.0.1`. Para dos equipos, cambia `Server IP` en el BoardManager de Level_Receiver por la dirección del emisor y guarda el nivel. Solo debe haber un Sender escuchando en el mismo puerto.

## Archivos

- `Source/FootballBoard`: campo, piezas y comunicación C++.
- `Content/FootballBoard`: niveles Sender y Receiver.
- `Scripts/setup_board.py`: creación inicial de escenas; protege niveles ya configurados y no debe ejecutarse de nuevo sobre ellos.

El césped y las fichas se generan al pulsar Play, por lo que el nivel en edición contiene principalmente luces y BoardManager.
