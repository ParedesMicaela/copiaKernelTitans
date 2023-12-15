#!/bin/bash
validate_ip() {
  # Regular expression pattern for IP address validation
  local ip_pattern='^([0-9]{1,3}.){3}[0-9]{1,3}$'

  if [[ $1 =~ $ip_pattern ]]; then
    return 0  # Valid IP address
  else
    return 1  # Invalid IP address
  fi
}

# Take the replace string
read -p "Ingrese kernel IP: " ipkernel
read -p "Ingrese Memoria IP: " ipMemoria
read -p "Ingrese File System IP: " ipFS
read -p "Ingrese CPU IP: " ipCPU

# Regexs
sKer="IP_KERNEL=[0-9\.]*"
sMem="IP_MEMORIA=[0-9\.]*"
sFs="IP_FILESYSTEM=[0-9\.]*"
sCpu="IP_CPU=[0-9\.]*"

# kernel
kc0="../kernel/cfg/kernel.config"
kc1="../deploy/Base/kernel_baseFIFO.config"
kc2="../deploy/Base/kernel_baseRR.config"
kc3="../deploy/Base/kernel_basePRIORIDADES.config"
kc4="../deploy/Recursos/kernel_recursos.config"
kc5="../deploy/Memoria/kernel_memoria.config"
kc6="../deploy/FileSystem/kernel_filesystem.config"
kc7="../deploy/Integral/kernel_integral.config"
kc8="../deploy/Estres/kernel_estres.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc6
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc7
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $kc8
# Search and replace CPU
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc0
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc1
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc2
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc3
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc4
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc5
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc6
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc7
sed -E -i "s/$sCpu/IP_CPU=$ipCPU/" $kc8
# Search and replace FS
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc0
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc1
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc2
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc3
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc4
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc5
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc6
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc7
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $kc8

# filesystem
filesystemc0="../filesystem/cfg/filesystem.config"
filesystemc1="../deploy/Base/filesystem_base.config"
filesystemc2="../deploy/Recursos/filesystem_recursos.config"
filesystemc3="../deploy/Memoria/filesystem_memoria.config"
filesystemc4="../deploy/FileSystem/filesystem_filesystem.config"
filesystemc5="../deploy/Integral/filesystem_integral.config"
filesystemc6="../deploy/Estres/filesystem_estres.config"
# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $filesystemc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $filesystemc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $filesystemc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $filesystemc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $filesystemc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $filesystemc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $filesystemc6

# cpu
cpc0="../cpu/cfg/cpu.config"
cpc1="../deploy/Base/cpu_base.config"

# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $cpc1

# memoria
mc0="../memoria/cfg/memoria.config"
mc1="../deploy/Base/memoria_base.config"
mc2="../deploy/Recursos/memoria_recursos.config"
mc3="../deploy/Memoria/memoria_memoriaFIFO.config"
mc4="../deploy/Memoria/memoria_memoriaLRU.config"
mc5="../deploy/FileSystem/memoria_filesystem.config"
mc6="../deploy/Integral/memoria_integral.config"
mc7="../deploy/Estres/memoria_estres.config"

# Search and replace Memoria
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc0
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc1
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc2
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc3
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc4
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc5
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc6
sed -E -i "s/$sMem/IP_MEMORIA=$ipMemoria/" $mc7

# Search and replace FS
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc0
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc1
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc2
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc3
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc4
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc5
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc6
sed -E -i "s/$sFs/IP_FILESYSTEM=$ipFS/" $mc7

echo "Las IPs han sido  modificadas correctamente"