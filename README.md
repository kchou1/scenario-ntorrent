Prerequisites
=============

Custom version of NS-3 and specified version of ndnSIM needs to be installed.

The code should also work with the latest version of ndnSIM, but it is not guaranteed.

    mkdir ndnSIM
    cd ndnSIM

    git clone -b ndnSIM-v2.5 https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
    git clone -b 0.18.0 https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
    git clone -b ndnSIM-2.5 --recursive https://github.com/named-data-ndnSIM/ndnSIM ns-3/src/ndnSIM

    # Build and install NS-3 and ndnSIM
    cd ns-3
    ./waf configure -d optimized
    ./waf
    sudo ./waf install

    if [[ `uname -a | grep Linux` ]]; then
        #When using Linux
        sudo ldconfig;
    elif [[ `uname -a | grep FreeBSD` ]]; then
        #When using FreeBSD
        sudo ldconfig -a;
    fi

    cd ..
    git clone --recursive https://github.com/kchou1/scenario-ntorrent.git scenario-ntorrent

    # Replace and Add files to ndnSim
    cp scenario-ntorrent/dapis/{forwarder.cpp,strategy.*,broadcast-strategy.*} ns-3/src/ndnSIM/NFD/daemon/fw

    cd scenario-ntorrent

    ./waf configure
    ./waf

After which you can proceed to compile and run the code

For more information how to install NS-3 and ndnSIM, please refer to http://ndnsim.net website.

Compiling
=========

To configure in optimized mode without logging **(default)**:

    ./waf configure

To configure in optimized mode with scenario logging enabled (logging in NS-3 and ndnSIM modules will still be disabled,
but you can see output from NS_LOG* calls from your scenarios and extensions):

    ./waf configure --logging

To configure in debug mode with all logging enabled

    ./waf configure --debug

If you have installed NS-3 in a non-standard location, you may need to set up ``PKG_CONFIG_PATH`` variable.

Running
=======

Normally, you can run scenarios either directly

    ./build/ntorrent-simple

or using waf

    ./waf --run ntorrent-simple

If NS-3 is installed in a non-standard location, on some platforms (e.g., Linux) you need to specify ``LD_LIBRARY_PATH`` variable:

    LD_LIBRARY_PATH=/usr/local/lib ./build/ntorrent-simple

or

    LD_LIBRARY_PATH=/usr/local/lib ./waf --run ntorrent-simple

To run scenario using debugger, use the following command:

    gdb --args ./build/ntorrent-simple


Running with visualizer
-----------------------

There are several tricks to run scenarios in visualizer.  Before you can do it, you need to set up environment variables for python to find visualizer module.  The easiest way to do it using the following commands:

    cd ns-dev/ns-3
    ./waf shell

After these command, you will have complete environment to run the vizualizer.

The following will run scenario with visualizer:

    ./waf --run ntorrent-simple --vis

or

    PKG_LIBRARY_PATH=/usr/local/lib ./waf --run ntorrent-simple --vis

If you want to request automatic node placement, set up additional environment variable:

    NS_VIS_ASSIGN=1 ./waf --run ntorrent-simple --vis

or

    PKG_LIBRARY_PATH=/usr/local/lib NS_VIS_ASSIGN=1 ./waf --run ntorrent-simple --vis

Available simulations
=====================

    ntorrent-simple
    ntorrent-multi-consumer
    ntorrent-fully-connected-consumer
    ntorrent-adhoc-sim
    ntorrent-adhoc-3nodes
---------------

A work in progress...
