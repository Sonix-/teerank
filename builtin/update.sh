#!/bin/sh

set -e

export PATH=.:"$PATH"

teerank-init
teerank-add-new-servers
teerank-remove-offline-servers 1
teerank-update-servers | teerank-update-players | teerank-update-clans
teerank-compute-ranks
