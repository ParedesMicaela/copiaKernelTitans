#!/bin/bash                                                                                                                                                                                                        

echo "Seleccione la prueba a realizar:"
echo "1)Base"
echo "2)Recursos"
echo "3)Memoria"
echo "4)File System"
echo "5)Integral"
echo "6)Estres"

read -p "Numero de prueba: " prueba

case $prueba in
    1)
        echo "Prueba Base"
        cp "../configs/Base/memoria_base.config" "../memoria/cfg/memoria.config"
        cp "../configs/Base/cpu_base.config" "../cpu/cfg/cpu.config"
        cp "../configs/Base/filesystem_base.config" "../filesystem/cfg/filesystem.config"

        echo "1. FIFO"
        echo "2. RR"
        echo "3. PRIORIDADES"
        read -p "Algoritmo a usar:" algoritmo
        case $algoritmo in
            1)
                cp "../configs/Base/kernel_baseFIFO.config" "../kernel/cfg/kernel.config"
                ;;
            2)
                cp "../configs/Base/kernel_baseRR.config" "../kernel/cfg/kernel.config"
                ;;
            3)
                cp "../configs/Base/kernel_basePRIORIDADES.config" "../kernel/cfg/kernel.config"
                ;;
        esac
        echo "Configuraciones seteadas correctamente para Prueba Base"
        ;;
    2)
        echo "Prueba Recursos"
        cp "../configs/Recursos/memoria_recursos.config" "../memoria/cfg/memoria.config"
        cp "../configs/Recursos/filesystem_recursos.config" "../filesystem/cfg/filesystem.config"
        cp "../configs/Recursos/kernel_recursos.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba Recursos"
        ;;
    3)
        echo "Prueba Memoria"
        cp "../configs/Memoria/kernel_memoria.config" "../kernel/cfg/kernel.config"
        cp "../configs/Memoria/filesystem_memoria.config" "../filesystem/cfg/filesystem.config"

        echo "1. FIFO"
        echo "2. LRU"
        read -p "Algoritmo a usar: " algMem
        case $algMem in
            1)
                cp "../configs/Memoria/memoria_memoriaFIFO.config" "../memoria/cfg/memoria.config"
                ;;
            2)
                cp "../configs/Memoria/memoria_memoriaLRU.config" "../memoria/cfg/memoria.config"
                ;;
        esac
        echo "Configuraciones seteadas correctamente para Prueba memoria"
        ;;

    4)
        echo "Prueba File System"
        cp "../configs/FileSystem/memoria_filesystem.config" "../memoria/cfg/memoria.config"
        cp "../configs/FileSystem/filesystem_filesystem.config" "../filesystem/cfg/filesystem.config"
        cp "../configs/FileSystem/kernel_filesystem.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba File System"
        ;;
    5)
        echo "Prueba Integral"
        cp "../configs/Integral/memoria_integral.config" "../memoria/cfg/memoria.config"
        cp "../configs/Integral/filesystem_integral.config" "../filesystem/cfg/filesystem.config"
        cp "../configs/Integral/kernel_integral.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba Integral"
        ;;
    6)
        echo "Prueba Estres"
        cp "../configs/Estres/memoria_estres.config" "../memoria/cfg/memoria.config"
        cp "../configs/Estres/filesystem_estres.config" "../filesystem/cfg/filesystem.config"
        cp "../configs/Estres/kernel_estres.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba Estres"
        ;;
    *)
        echo "Comando no reconocido"
        ;;
esac

exit 0





