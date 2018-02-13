export CTP7_MOD_ROOT=${PWD}
if [[ ! "$0" =~ ("bash") ]]
then
    # best for calling from zsh                                                                                                                                                                                                                                                   
    export CTP7_MOD_ROOT=$(dirname $(readlink -f "$0") )
else
    # bash specific, must be sourced from a bash shell...                                                                                                                                                                                                                         
    export CTP7_MOD_ROOT=$(dirname $(readlink -f "${BASH_SOURCE[0]}"))
fi
