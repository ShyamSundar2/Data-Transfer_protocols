Command to generate all executables : make all/make

Command To remove all executables : make clean

Proccess to Simulate Selective Repeat:

First execute Receiver - ./ReceiverSR (-d) -p <int> -N <int> -n <int> -W <int> -B <int> -e <double>

In other terminal
execute Sender - ./SenderSR (-d) -s <string> -p <int> -n <int> -L <int> -R <int> -N <int> -W <int> -B <int>

Proccess to Simulate Go-Back-N:

First execute Receiver - ./ReceiverGBN (-d) -p <int> -n <int> -e <int>

In other terminal
execute Sender - ./SenderGBN (-d) -s <string> -p <int> -l <int> -r <int> -n <int> -w <int> -b <int>
