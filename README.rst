===========
QEMU README
===========

QEMU is a generic and open source machine & userspace emulator and
virtualizer.

Add FR80 CPUs.

Building
========

.. code-block:: shell

  mkdir build
  cd build
  ../configure --enable-debug --target-list=fr-softmmu --disable-kvm
  make

Running
========

.. code-block:: shell

  ./fr-softmmu/qemu-system-fr -nographic -kernel A.bin
