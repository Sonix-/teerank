#!/bin/sh

mkdir -p "$TEERANK_ROOT"
mkdir -p "$TEERANK_ROOT/servers"
mkdir -p "$TEERANK_ROOT/players"
mkdir -p "$TEERANK_ROOT/clans"
mkdir -p "$TEERANK_ROOT/pages"
[ -f "$TEERANK_ROOT/version" ] || echo "4" > "$TEERANK_ROOT/version"
