Пример запуска мининет:

sudo mn --topo $YOUR_TOPO --switch ovsk,protocols=OpenFlow13 --controller remote,ip=$CONTROLLER_IP,port=6653

sudo mn --topo linear,2 --switch ovsk,protocols=OpenFlow13 --controller remote,ip=0.0.0.0,port=6653
