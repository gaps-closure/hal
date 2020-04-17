# Checkout and compile CLOSURE hal and apps
# git clone ...

# Mercury instructions for RHEL
# scl enable rh-perl526 bash
# scl enable devtoolset-8 bash
# scl enable rh-python36 bash
# export PATH=/opt/cmake3-17/bin:$PATH
# export PATH=/opt/openssl/bin:$PATH
# export LD_LIBRARY_PATH=/opt/openssl/lib:$LD_LIBRARY_PATH
# for your python3 scripts I had to change over to using
#  #!/usr/bin/env python3

# Insmod the ilip and pcie modules
# The current running drivers are located at /home/gaps/Mercury5/pcie
# On gaps-2-1 and gaps-2-2
# The closure and pirate users are part of the gaps group and also have sudo
# privileges so they can load and unload the driver.
# Note gaps-2-2 can only send to gaps-2-1 via PCIe, return path is via the direct
# connect Ethernet 

# To load the driver
# for the loopback driver just change the gaps_pcie.ko to gaps_loopback.ko in the above.
# Note the order is important and must be maintained.
#
# sudo insmod gaps_pcie.ko
# sudo insmod gaps_ilip.ko

# To unload the driver
#
# sudo rmmod gaps_ilip.ko
# sudo rmmod gaps_pcie.ko

# delete, then create two taps
# assign addresses per mercury config 

ip link set dev ltap0 down > /dev/null 2>&1
ip link set dev rtap0 down > /dev/null 2>&1
ip link delete ltap0 > /dev/null 2>&1
ip link delete rtap0 > /dev/null 2>&1

ip tuntap add mode tap ltap0
ip tuntap add mode tap rtap0
ip link set dev ltap0 up
ip link set dev rtap0 up
ip addr add 169.254.0.10/24 dev ltap0
ip addr add 169.254.0.20/24 dev rtap0

# Note gaps_2_1 node has address 169.254.0.10
# Note gaps_2_2 node has address 169.254.0.20
# The PCIe device allows data from gaps_2_2 to gaps_2_1 only 
# The passthru is to be used for gaps_2_1 to gaps_2_2 traffic
# Confirm name of PCIe/ILIP device is /dev/gaps_ilip_1_read etc. as before
# Confirm IP addresses on testbed before proceeding, may need to change configs

# start HAL for orange and green on gaps_2_1 and gaps_2_2 respectively
# ~/gaps/hal$ daemon/hal test/be-integrated-demo-20200420/forward-mission-app/mstest_green_gaps_2_2.cfg
# ~/gaps/hal$ daemon/hal test/be-integrated-demo-20200420/forward-mission-app/mstest_orange_gaps_2_1.cfg

# start orange and green applications 
