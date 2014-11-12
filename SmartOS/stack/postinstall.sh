#! /bin/ksh

case "$1" in
'red')
    /stacktest $1
    ;;

'corrupt')
    /stacktest $1
    ;;
*)
    echo "Usage: $0 { red | corrupt }"
    exit 1
    ;;
esac
