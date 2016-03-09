
source /opt/intel/bin/compilervars.sh intel64

# These mic debuging flag may cause problems when running together with FairMQ
#export OFFLOAD_REPORT=3
#export H_TRACE=1

fairsoft_path="/home/andrey/soft/fair/FairSoftInstall_gcc"
fairroot_path="/home/andrey/Development/fairroot/install"
client_path="${fairroot_path}/bin/ex5-client-xeonphi"
server_path="${fairroot_path}/bin/ex5-server-xeonphi"
config_path="/home/andrey/Development/fairroot/FairRoot/examples/MQ/5-req-rep-xeonphi/ex5-req-rep.json"

export LD_LIBRARY_PATH="${fairsoft_path}/lib:${fairroot_path}/lib:${LD_LIBRARY_PATH}"

xterm -hold -e "${client_path} --id client --config-json-file ${config_path}"&
xterm -hold -e "${server_path} --id server --config-json-file ${config_path}"&
