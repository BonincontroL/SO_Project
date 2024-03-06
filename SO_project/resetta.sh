#!/bin/bash

pkill nave.out 
pkill porto.out
pkill master.out

# Rimuovere tutte le code di messaggi
ipcs -q | awk '{if ($1 ~ /^[0-9]/) print "-q " $2}' | xargs ipcrm

# Rimuovere tutte le shared memory segments
ipcs -m | grep '^0x' | awk '{print "-m " $2}' | xargs ipcrm

# Rimuovere tutti i semafori
ipcs -s | awk '{if ($1 ~ /^[0-9]/) print "-s " $2}' | xargs ipcrm

# Rimuovere tutti i semafori POSIX
ipcs -s -p | awk '{if ($1 ~ /^[0-9]/) print $1, $2}' | xargs -n 2 ipcrm

echo "Tutte le risorse IPC rimosse con successo."
