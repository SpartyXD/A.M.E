# A.M.E BOT
Robot de compañia tanto para estudiar como para jugar.

La idea inicial surgio debido a que era el cumple de mi mejor amiga y no sabia que regalarle, asi que dije: "Por que no le armo un robot?" jskdjs

## Proceso de desarrollo
El robot fue modelado y armado en 1 semana aprox, con ayuda de buenas dosis de azucar y sus buenas trasnochadas :D

![Robot ya armado](https://github.com/user-attachments/assets/514cfe85-05a9-4405-8938-bfee4e6ffb10)

![Mockups iniciales del robot](https://github.com/user-attachments/assets/9903c608-9f89-4afa-b77a-d2a0b1c458d2)

![Circuiteria interna robot](https://github.com/user-attachments/assets/3d10ab85-f5f2-4d05-95e1-1b0f57292e02)

## Implementacion del codigo
El codigo esta listo para implementarse como proyecto en PlatformIO (extension de Visual studio code) y 
esta casi totalmente modularizado y parametrizado, en caso de querer cambiar timers, sonidos, caras, mensajes, etc.

## Modos

### Idle
El robot existe, tira una que otra frase motivadora y cambia sus caras de vez en cuando

### Birthday
Canta feliz cumpleaños (de los propositos principales jskdj)

### Timer
Accede a un timer programable que llega a maximo 3 horas, una vez el tiempo se acaba
se activa una alarma y el bot empieza a moverse para avisar el termino del tiempo.

### Pong
Recrea el clasico juego PONG. La partida consiste en jugar contra el bot, quien dependiendo de la 
**dificultad** seleccionada, hara que la pelota sea mas rapida y este sera mas o menos preciso.

El jugador controla su parte (izquierda) usando el brazo izquierdo del bot como palanca.
Se puede configurar la puntuacion ganadora (por defecto es 3)

### Gambling
Modo para los indecisos: Si alguna vez tienes que decidir entre 2 o mas opciones, simplemente dile al bot y el
escogera una al azar por ti. (funciona igual que las maquinas de azar del casino, bajando una palanca lateral)
