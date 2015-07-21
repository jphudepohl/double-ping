# double-ping

This is an NDN application designed to investigate the difference in travel times for interest packets in relation to data packets. The files should be put in the ndn-cxx/examples directory and can be built using the instructions found at: https://github.com/named-data/ndn-cxx/tree/master/examples.

To use, start Server A on one machine: 
```
# From the ndn-cxx directory on Machine A
./build/examples/serverA [prefix to advertise]
```
and start Server B on the other machine:
```
# From the ndn-cxx directory on Machine B
./build/examples/serverA [prefix to advertise]
```
Then, on the same machine as server A, start the ping client:
```
# From the ndn-cxx directory on Machine A
./build/examples/startDoublePing [name to ping]
```
Note that `[name to ping]` should begin with the same prefix that Server B is advertising. For example, if Server B is advertising `/serverB`, `[name to ping]` could be `/serverB/interest1`.

This code has been adapted from the ndn-cxx consumer and producer applications (https://github.com/named-data/ndn-cxx/tree/master/examples), as well as the ndn-tools implementation of ndnping (https://github.com/named-data/ndn-tools/tree/master/tools/ping).
