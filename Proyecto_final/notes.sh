#!/bin/bash 

greenColour="\e[0;32m\033[1m"
endColour="\033[0m\e[0m"
redColour="\e[0;31m\033[1m"
blueColour="\e[0;34m\033[1m"
yellowColour="\e[0;33m\033[1m"
purpleColour="\e[0;35m\033[1m"
turquoiseColour="\e[0;36m\033[1m"
grayColour="\e[0;37m\033[1m"

echo -e "${redColour}[!] Proyecto final${endColour}\n"

echo -e "${blueColour} Hacer un sistema denominado examenes en linea el cual debera funcionar como sigue:${endColour}\n"

echo -e "${purpleColour}[-] Un usuario se conectará a un servidor el cual solicitará la información siguiente: \n${endColour}${redColour}[!]${redColour}${purpleColour} Matricula y contraseña.${endColour}\n${redColour}[!]${endColour}${purpleColour} El servidor tendrpa un banco de materias como mínimo de 20 usuarios${endColour}\n${redColour}[!]${endColour}${purpleColour} Una vez validad la información se le dará acceso al usuario a un menú de 3 examenes a realizar${endColour}${redColour} (I. Matematicas, II. Español e III. Ingles)${endColour}\n${redColour}[!]${endColour} ${purpleColour}El usuario seleccionará una opción de las 3 disponibles y procederá a contestar dicho examen${endColour}\n${redColour}[!]${endColour}${purpleColour} Cada examen tendrá un banco de preguntas ${endColour}\n${redColour}[!]${endColour}${purpleColour} Cada pregunta es de opción multiple con 3 opciones de respuesta y solo uno de ellas es la correcta${endColour}\n${redColour}[!]${endColour} ${purpleColour}Cada pregunta tendrá como tolerancia de respuesta ${endColour}${yellowColour}10 minutos${endColour}.${purpleColour}Si no se contesta se tomará como no correcta y seguirá a la siguiente pregunta o finaliza el examen${endColour}\n${redColour}[!]${endColour}${purpleColour} El examen constará de 10 preguntas aleatorias y 2 usuarios no pueden tener el mismo examen, pero de coincidir en algunas preguntas\n${redColour}[!]${endColour} ${purpleColour}Al finalizar el examen el servidor dará una calificación final al usuario${endColour}\n${redColour}[!]${endColour}${purpleColour} Después de que 3 usuarios hayan realizado examen el servidor mostrará en pantall los resultados de los 3 usuarios${endColour}\n${redColour}[!] Cada usuario realizará los 3 examenes correspondientes${endColour}"

echo -e "\n${turquoiseColour}[!] Utilizar un entorno gráfico... de preferencia GTK${endColour}"
