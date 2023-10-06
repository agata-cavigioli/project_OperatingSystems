# PandOS Fase 2: Kernel del SO

Fase 2 del progetto PandOS per il corso di Sistemi Operativi 2020/21.
L'architettura di PandOS è basata su sei livelli di astrazione e con la fase 2 ci occupiamo del livello 3, ossia l'implementazione del kernel del sistema operativo.
Sono stati realizzati uno scheduler, un excpetion handler, un syscall handler e un interrupt handler.

## Requisiti

- Il simulatore di una macchina virtuale [µMPS3](https://github.com/virtualsquare/umps3).
- Il comando make.

## Installazione

Estrarre i file in progetto.tar.gz.

Il file contiene:

- AUTHORS		autori
- README.md	        (questo file)
- src		        directory sorgenti
  - initial.c	        inizializzazione del kernel
  - p2test.c            test file del progetto
  - include	        directory include
    - h		        directory headers
    - phase1	        directory sorgenti della fase 1
    - phase2	        directory sorgenti della fase 2

Dalla directory dell'estrazione, per compilare i sorgenti, eseguire:

```
$ cd src
$ make
```
Lanciare umps3 e creare una nuova configurazione nella cartella src con i sorgenti compilati.

Far partire la macchina virtuale ed eseguire il programma.

Il comando ```make``` compila automaticamente l'ultima fase disponibile; è però possibile compilare ed eseguire singolarmente le singole fasi con i comandi ```make phase1```, ```make phase2``` etc.

## Segnalazioni

Per il corretto funzionamento è necessario modificare le impostazioni della macchina virtuale come segue:

* A macchina spenta, selezionare ```Simulator -> Edit configuration...```;
* Alla voce ```TLB Floor Address``` selezionare l'opzione ```0x8000.0000```;
* Salvare le modifiche premendo ```OK```.

## Licenza

[GNU GPLv3](https://choosealicense.com/licenses/gpl-3.0/)
