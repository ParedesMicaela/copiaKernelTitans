/*
Las acciones que se pueden realizar en cuanto a los archivos van a venir por parte de la CPU
como llamadas por medio de las funciones correspondientes a archivos.

El Kernel deberá implementar una tabla global de archivos abiertos,
para gestionar los mismos como si fueran un recurso de una única instancia.


Las operaciones de Filesystem se dividirán en dos tipos de locks obligatorios:
    -lock de lectura (R)
    -lock de escritura (W)
    (locks estarán identificados en la función F_OPEN y finalizarán con la función F_CLOSE)
    (Se tendrá un único lock de lectura (compartido por todos los F_OPEN en modo lectura)
    y un lock exclusivo por cada pedido de apertura que llegue en modo escritura)

El lock de lectura
    -se resolverá sin ser bloqueado siempre y cuando no exista activamente un lock de escritura.

Al momento en el que se active el lock de escritura:
    -encolar locks de lectura o escritura que se requieran para dicho archivo.
    -lock se bloqueará hasta que se resuelvan:
        --pedidos que participen en lock de archivo que se encontraban activos.


Cada proceso va a contar con su propio puntero:
    -le permitirá desplazarse por el contenido del archivo utilizando F_SEEK
    -luego si el tipo de lockeo lo permite:
        --efectuar una lectura (F_READ) o escritura (F_WRITE) sobre el mismo 


Para ejecutar estas funciones va a recibir:
    -el Contexto de Ejecución de la CPU
    -la función como motivo.
        --El accionar de cada función va a ser el siguiente:
        --//funciones de "kernel_manejo_fs_funciones.c"
        
*/