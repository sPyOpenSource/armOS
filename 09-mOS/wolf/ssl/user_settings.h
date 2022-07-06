    //#include <stddef.h>
    //#define MBED
    #define WOLFSSL_CMSIS_RTOS    
    #define WOLFSSL_USER_IO
    #define NO_WRITEV
    #define NO_DEV_RANDOM
    #define HAVE_ECC
    #define HAVE_AESGCM
    
    #define WOLFSSL_SHA384
    #define WOLFSSL_SHA512
    #define HAVE_CURVE25519
    #define HAVE_ED25519   /* with HAVE_SHA512 */
    //#define HAVE_POLY1305
    //#define HAVE_CHACHA
    //#define HAVE_ONE_TIME_AUTH
    
    #define NO_SESSION_CACHE // For Small RAM

    #define NO_WOLFSSL_DIR  
    #define DEBUG_WOLFSSL

    #define HAVE_SUPPORTED_CURVES    
    #define HAVE_TLS_EXTENSIONS
    #define HAVE_HKDF
    #define WC_RSA_PSS
    #define WOLFSSL_TLS13
    
    #define SIZEOF_LONG_LONG  8
    /* Options for Sample program */
    //#define WOLFSSL_NO_VERIFYSERVER
    //#define NO_FILESYSTEM
    //#ifndef WOLFSSL_NO_VERIFYSERVER
        #define TIME_OVERRIDES
        //#define HAVE_TM_TYPE
        #define XTIME time
        #define XGMTIME localtime
    //#endif