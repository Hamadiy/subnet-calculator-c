# Calculateur de sous-réseaux IPv4 & VLSM en C

![CI](https://github.com/Hamadiy/subnet-calculator-c/actions/workflows/ci.yml/badge.svg)
![Langage](https://img.shields.io/badge/langage-C11-blue)

Outil en ligne de commande pour le **subnetting IPv4** et la **planification VLSM**. Il automatise les calculs d'un plan d'adressage, qu'on fait habituellement à la main en TP ou pour un projet Cisco Packet Tracer.

## Fonctionnalités

- **Détails d'un réseau** à partir d'une notation CIDR : masque, wildcard (utile pour les ACL et OSPF), adresse réseau, broadcast, plage d'hôtes, nombre d'hôtes, classe, adresse privée ou publique.
- **Plan VLSM** : à partir d'un réseau et d'une liste de besoins en hôtes, l'outil attribue des sous-réseaux du plus grand au plus petit, ce qui garantit qu'ils sont alignés sans chevauchement.
- Gestion des cas particuliers : `/31` (lien point à point, RFC 3021) et `/32`.
- Validation stricte des entrées (`256.1.1.1`, `/33`, etc. sont refusés).
- Bibliothèque `ipv4.c` séparée de l'interface et couverte par **plus de 50 tests unitaires**.

## Compilation

```bash
make        # produit bin/ipcalc
make test   # tests unitaires + exemple VLSM
```

## Utilisation

### Détails d'un réseau

```
$ ./bin/ipcalc 192.168.1.130/26
Address    : 192.168.1.130
Netmask    : 255.255.255.192 = /26
Wildcard   : 0.0.0.63
Network    : 192.168.1.128/26
Broadcast  : 192.168.1.191
First host : 192.168.1.129
Last host  : 192.168.1.190
Hosts      : 62
Class      : C (private)
```

### Plan VLSM

Exemple : quatre LAN de 50, 20, 10 et 2 hôtes (le dernier pour une liaison entre routeurs), à découper dans `192.168.0.0/24`.

```
$ ./bin/ipcalc -v 192.168.0.0/24 50 20 10 2
LAN     Needed  Usable  Subnet              Host range                        Broadcast
LAN2        50      62  192.168.0.0/26      192.168.0.1 - 192.168.0.62        192.168.0.63
LAN4        20      30  192.168.0.64/27     192.168.0.65 - 192.168.0.94       192.168.0.95
LAN1        10      14  192.168.0.96/28     192.168.0.97 - 192.168.0.110      192.168.0.111
LAN3         2       2  192.168.0.112/30    192.168.0.113 - 192.168.0.114     192.168.0.115

Addresses used: 116 / 256
```

`LANn` correspond à l'ordre des besoins sur la ligne de commande.

## Algorithme VLSM

1. Trier les besoins par nombre d'hôtes décroissant.
2. Pour chacun, prendre le plus petit préfixe tel que 2^(32−p) − 2 ≥ hôtes.
3. Allouer les blocs les uns après les autres depuis l'adresse réseau. Comme les blocs sont des puissances de 2 et vont du plus grand au plus petit, chaque sous-réseau commence automatiquement sur une frontière valide.
4. Échouer proprement si l'espace d'adressage est insuffisant.

## Structure

```
src/ipv4.h / ipv4.c   bibliothèque (parsing, masques, VLSM)
src/main.c            interface en ligne de commande
tests/test_ipv4.c     tests unitaires
```

## Auteur

**Younesse HAMADI** – étudiant en BUT Réseaux & Télécommunications.
