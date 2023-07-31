#!/usr/bin/env bash

function main () {
    if (($# != 4)); then
        echo -e "\e[1;91mIngrese los parametros correctos: <IP_KERNEL> <IP_CPU> <IP_MEMORIA> <IP_FILESYSTEM>\e[0m"
        exit 1
    fi

    local -r IPKernel=$1
    local -r IPCpu=$2
    local -r IPMemoria=$3
    local -r IPFileSystem=$4

    # Cambiar IP de memoria, fileSystem, cpu y kernel en kernel
    perl -pi -e "s/(?<=IP_MEMORIA=).*/${IPMemoria}/g" kernel/cfg/*
    perl -pi -e "s/(?<=IP_FILESYSTEM=).*/${IPFileSystem}/g" kernel/cfg/*
    perl -pi -e "s/(?<=IP_CPU=).*/${IPCpu}/g" kernel/cfg/*
    perl -pi -e "s/(?<=IP_KERNEL=).*/${IPKernel}/g" kernel/cfg/*

    # Cambiar IP de memoria y cpu en cpu
    perl -pi -e "s/(?<=IP_MEMORIA=).*/${IPMemoria}/g" cpu/cfg/*
    perl -pi -e "s/(?<=IP_CPU=).*/${IPCpu}/g" cpu/cfg/*
    
    # Cambiar IP de memoria y filesystem en fileSystem
    perl -pi -e "s/(?<=IP_MEMORIA=).*/${IPMemoria}/g" fileSystem/cfg/*
    perl -pi -e "s/(?<=IP_FILESYSTEM=).*/${IPFileSystem}/g" fileSystem/cfg/*

     # Cambiar IP de memoria en memoria
    perl -pi -e "s/(?<=IP_MEMORIA=).*/${IPMemoria}/g" memoria/cfg/*

    # Cambiar IP de kernel en consola
    perl -pi -e "s/(?<=IP_KERNEL=).*/${IPKernel}/g" consola/cfg/*
    
}

main "$@"