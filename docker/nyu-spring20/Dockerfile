FROM multi2sim/multi2sim:build-env

# Build Multi2Sim
RUN cd /tmp && git clone https://github.com/robbiefrost/multi2sim && git clone https://github.com/Multi2Sim/m2s-bench-splash3 && cd multi2sim && libtoolize && aclocal && autoconf && automake --add-missing && ./configure && make && make install
