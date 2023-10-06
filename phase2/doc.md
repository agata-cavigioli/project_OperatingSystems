# Principali scelte implementative

Di seguito riportiamo alcuni particolari implementativi.

## Process time

Il process time viene aggiornato ogni volta che il processo viene bloccato, sia dallo scheduler, sia da una syscall.
Ad ogni time slice viene memorizzato il TOD attuale; ad ogni eccezione viene memorizzato il TOD al momento dell'invocazione dell'handler. Ciò permette di aggiungere al process time già calcolato l'intervallo ```exception_time - time_slice_start``` e tenere correttamente traccia del tempo trascorso.

## Gestione delle system calls

Per motivi organizzativi è stato deciso di racchiudere l'handler delle syscall in una apposita libreria che offre al resto del kernel sia l'handler, sia le singole funzioni ausiliarie, che possono essere invocate separatamente per determinate operazioni (e.g. P e V sui semafori).
L'handler è stato concepito in modo da poter essere invocato senza parametri aggiuntivi e da realizzare autonomamente tutte le operazioni necessarie alla corretta gestione della system call.

## Semafori dei device

Si dispongono di 5 devices (disk, flash, network, printer e terminal) per 8 istanze, per un totale di 40. Dal momento che il terminale deve gestire sia lo stadio di scrittura che quello di lettura (due sub devices), i semafori necessari per sincronizzare le operazioni di I/O sono 48.

Ogni device occupa una linea di interrupt specifica per ogni sua istanza, pertanto è stato deciso di organizzare i loro semafori in due array bidimensionali per garantire un veloce e intuitivo accesso:

- `deviceSemaphores[4][8]`. Dove 4 sono i device, rispettivamente, disk, flash, network e printer e 8 sono le istanze per una linea di interrupt. Avendo noto l'offset che determina l'inizio delle linee di interrupt assegnati ai device, è possibile accedere al semaforo interessato con `deviceSemaphores[deviceLineNo-deviceLineOffset][deviceNo]`.
- `terminalDeviceSemaphores[2][8]`. Dove 2 sono i due stadi (o sub devices) del terminale, rispettivamente scrittura e lettura, e 8 sono le istanze per la sua linea di interrupt. È possibile accedere al semaforo interessato con `terminalDeviceSemaphores[terminalSubDevice][terminalNo]`.

Infine, l'interval timer interrupt ha un suo semaforo singolo: `pseudoClockSemaphore`.
