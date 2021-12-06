vm1 {
    cpus = 1;
    origin = 4;
    mode = "merge";
    sub {
        a = "b";
        c = "d";
        foo = "bar";
    }
    from = "test";
}
vm2 {
    cpus = 4;
}
vm3 {
    cpus = 2;
    kilo = 1000;
    kibbi = 1024;
}
