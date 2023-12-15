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
        cp "../deploy/Base/memoria_base.config" "../memoria/cfg/memoria.config"
        cp "../deploy/Base/cpu_base.config" "../cpu/cfg/cpu.config"
        cp "../deploy/Base/filesystem_base.config" "../filesystem/cfg/filesystem.config"

        echo "1. FIFO"
        echo "2. RR"
        echo "3. PRIORIDADES"
        read -p "Algoritmo a usar:" algoritmo
        case $algoritmo in
            1)
                cp "../deploy/Base/kernel_baseFIFO.config" "../kernel/cfg/kernel.config"
                ;;
            2)
                cp "../deploy/Base/kernel_baseRR.config" "../kernel/cfg/kernel.config"
                ;;
            3)
                cp "../deploy/Base/kernel_basePRIORIDADES.config" "../kernel/cfg/kernel.config"
                ;;
        esac
        echo "Configuraciones seteadas correctamente para Prueba Base"
        ;;
    2)
        echo "Prueba Recursos"
        cp "../deploy/Recursos/memoria_recursos.config" "../memoria/cfg/memoria.config"
        cp "../deploy/Recursos/filesystem_recursos.config" "../filesystem/cfg/filesystem.config"
        cp "../deploy/Recursos/kernel_recursos.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba Recursos"
        ;;
    3)
        echo "Prueba Memoria"
        cp "../deploy/Memoria/kernel_memoria.config" "../kernel/cfg/kernel.config"
        cp "../deploy/Memoria/filesystem_memoria.config" "../filesystem/cfg/filesystem.config"

        echo "1. FIFO"
        echo "2. LRU"
        read -p "Algoritmo a usar: " algMem
        case $algMem in
            1)
                cp "../deploy/Memoria/memoria_memoriaFIFO.config" "../memoria/cfg/memoria.config"
                ;;
            2)
                cp "../deploy/Memoria/memoria_memoriaLRU.config" "../memoria/cfg/memoria.config"
                ;;
        esac
        echo "Configuraciones seteadas correctamente para Prueba memoria"
        ;;

    4)
        echo "Prueba File System"
        cp "../deploy/FileSystem/memoria_filesystem.config" "../memoria/cfg/memoria.config"
        cp "../deploy/FileSystem/filesystem_filesystem.config" "../filesystem/cfg/filesystem.config"
        cp "../deploy/FileSystem/kernel_filesystem.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba File System"
        ;;
    5)
        echo "Prueba Integral"
        cp "../deploy/Integral/memoria_integral.config" "../memoria/cfg/memoria.config"
        cp "../deploy/Integral/filesystem_integral.config" "../filesystem/cfg/filesystem.config"
        cp "../deploy/Integral/kernel_integral.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba Integral"
        ;;
    6)
        echo "Prueba Estres"
        cp "../deploy/Estres/memoria_estres.config" "../memoria/cfg/memoria.config"
        cp "../deploy/Estres/filesystem_estres.config" "../filesystem/cfg/filesystem.config"
        cp "../deploy/Estres/kernel_estres.config" "../kernel/cfg/kernel.config"
        echo "Configuraciones seteadas correctamente para Prueba Estres"
        ;;
    *)
        echo "Comando no reconocido"
        ;;
esac

exit 0





