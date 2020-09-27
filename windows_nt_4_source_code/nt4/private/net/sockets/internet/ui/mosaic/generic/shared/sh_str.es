/*
        sh_str.stb

        This file is used to generate shared strings IDs.  It is also included in the
        platform-specific string table.  The order of strings does not matter. sh_sid.h is
        generated from this file.

        DO NOT USE COMMAS IN COMMENTS BECAUSE THEY ARE TREATED AS DELIMITERS.
        THE STRING GENERATION UTILITY IS NOT INTELLIGENT.
*/

/********************************************************************
        Informational messages
********************************************************************/


GTR_SID(SID_INF_CONNECTING_TO_HTTP_SERVER, "Se est� conectando con el servidor HTTP")
GTR_SID(SID_INF_SENDING_COMMAND, "Se est� enviando el comando")
GTR_SID(SID_INF_PROCESSING_SERVER_RESPONSE, "Se est� procesando la respuesta del servidor")
GTR_SID(SID_INF_FINDING_ADDRESS_FOR_SYSTEM_S,                           "Se est� buscando la direcci�n para el sistema '%s'")
GTR_SID(SID_INF_FETCHING_IMAGE_S, "Se est� recogiendo la imagen '%s'")
GTR_SID(SID_INF_LOADING_IMAGES, "Se est�n cargando im�genes")
GTR_SID(SID_INF_ACCESSING_URL, "Se est� accediendo al URL: ")

GTR_SID(SID_INF_CONNECTING_TO_FTP_SERVER, "Se est� conectando con el servidor FTP")
GTR_SID(SID_INF_LOGGING_INTO_FTP_SERVER, "Se est� accediendo al servidor FTP")
GTR_SID(SID_INF_ESTABLISHING_FTP_CONNECTION,                            "Se est� estableciendo la conexi�n con los datos FTP")
GTR_SID(SID_INF_SENDING_FTP_COMMANDS, "Se est�n enviando comandos FTP")
GTR_SID(SID_INF_RECEIVING_FTP_DIRECTORY_LISTING,                        "Se est� recibiendo el listado de directorios del servidor FTP")

GTR_SID(SID_INF_CONNECTING_TO_GOPHER_SERVER,                            "Se est� conectando con el servidor gopher")
GTR_SID(SID_INF_SENDING_GOPHER_COMMANDS, "Se est�n enviando comandos gopher")
GTR_SID(SID_INF_RECEIVING_GOPHER_DATA, "Se est�n recibiendo datos gopher")

GTR_SID(SID_INF_CONNECTING_TO_MAIL_SERVER, "Se est� conectando con el servidor de correo")

GTR_SID(SID_INF_SECURING_DOCUMENT, "Se est� protegiendo el documento para la transmisi�n")
GTR_SID(SID_INF_LOADING_SECURED_DOCUMENT, "Se est� cargando el documento protegido")
GTR_SID(SID_INF_OPENING_SECURED_DOCUMENT, "Se est� abriendo el documento protegido")

GTR_SID(SID_INF_PROCESSING_AIFF_FILE, "Se est� procesando el archivo de sonido AIFF")
GTR_SID(SID_INF_PROCESSING_AU_FILE, "Se est� procesando el archivo de sonido AU")

GTR_SID(SID_INF_SESSION_HISTORY, "<title>Historial de la sesi�n</title>\n")
GTR_SID(SID_INF_SESSION_HISTORY_FOR_DATE_S, "\n<h1>Historial de sesi�n para %s</h1>\n")

GTR_SID(SID_INF_GLOBAL_HISTORY, "<title>Historial global</title>\n")
GTR_SID(SID_INF_GLOBAL_HISTORY_PAGE, "\n<h1>P�gina del historial global</h1>\n")

GTR_SID(SID_INF_HOTLIST, "<title>Lista</title>\n")
GTR_SID(SID_INF_HOTLIST_PAGE, "\n<h1>P�gina de la lista</h1>\n")

GTR_SID(SID_INF_AVAIALBLE_NEWSGROUPS, "Grupos de noticias disponibles")
GTR_SID(SID_INF_NEWSGROUP_ARTICLES_S_D_D, "Grupo de noticias %s (Art�culos %d - %d)")
GTR_SID(SID_INF_ARTICLES_CURRENTLY_SHOWN_D_S_D_D, "Hay %d art�culos en %s. Se muestran los art�culos %d - %d.")
GTR_SID(SID_INF_EARLIER_ARTICLES, "Art�culos anteriores")
GTR_SID(SID_INF_LATER_ARTICLES, "Art�culos posteriores")
GTR_SID(SID_INF_NO_SUBJECT, "(Ning�n tema)")
GTR_SID(SID_INF_NEWSGROUPS, "Grupos de noticias:")
GTR_SID(SID_INF_REFERENCES, "Referencias:")
GTR_SID(SID_INF_CONNECTING_TO_NEWS_SERVER, "Se est� conectando con el servidor de noticias")
GTR_SID(SID_INF_RETRIEVING_NEWS_ARTICLE, "Se est� recuperando el art�culo")
GTR_SID(SID_INF_RETRIEVING_NEWS_GROUP_LIST, "Se est� recuperando la lista de grupos de noticias")
GTR_SID(SID_INF_RETRIEVING_NEWS_ARTICLE_LIST,                           "Se est� recuperando la lista de art�culos")

GTR_SID(SID_INF_DECOMPRESSING_JPEG_IMAGE, "Se est� descomprimiendo la imagen JPEG")
GTR_SID(SID_INF_DOCUMENT_MOVED, "Documento desplazado")
GTR_SID(SID_INF_DOWNLOADING, "Se est� cargando: ")
GTR_SID(SID_INF_RETRIEVING_HTTP_HEAD_INFORMATION,                       "Se est� recuperando informaci�n HTTP HEAD: ")
GTR_SID(SID_INF_LOADING_MAPS_FROM, "Se est�n cargando mapas desde ")
GTR_SID(SID_INF_FORMATTING_PLAIN_TEXT, "Se est� formateando texto normal")
GTR_SID(SID_INF_CONVERTING_SELECTION_TO_PLAIN_TEXT,                     "Se est� convirtiendo la selecci�n en texto normal")

/********************************************************************
        Dialog/caption strings
********************************************************************/

GTR_SID(SID_DLG_NO_DOCUMENT, "(Ning�n documento)")
GTR_SID(SID_DLG_UNTITLED, "Sin t�tulo")
GTR_SID(SID_DLG_MISSING_IMAGE_HOLDER_STRING, "(Imagen)")
GTR_SID(SID_DLG_SEARCHABLE_INDEX_ENTER_KEYWORD,                         "En este �ndice se pueden realizar b�squedas. Escriba una o varias palabras clave: ")
GTR_SID(SID_DLG_LESS_THAN_1000_BYTES_L, "%ld bytes")
GTR_SID(SID_DLG_LESS_THAN_10000_BYTES_L_L, "%ld,%03ld bytes")
GTR_SID(SID_DLG_MEGABYTES_L_L, "%ld.%ld MB")
GTR_SID(SID_DLG_SAVE_SESSION_HISTORY_TITLE, "Grabar historial de sesi�n")


/********************************************************************
        General error messages
********************************************************************/

GTR_SID(SID_ERR_SIMPLY_SHOW_ARGUMENTS_S_S, "%s%s")
GTR_SID(SID_ERR_OUT_OF_MEMORY, "No hay memoria suficiente para realizar la operaci�n solicitada. Cierre aplicaciones que no est� utilizando e int�ntelo de nuevo.")
GTR_SID(SID_ERR_FILE_NOT_FOUND_S, "No se ha encontrado '%s'.")
GTR_SID(SID_ERR_ONE_ADDITIONAL_ERROR_OCCURRED,                          "Se ha producido un error adicional.")
GTR_SID(SID_ERR_ADDITIONAL_ERRORS_OCCURRED_L,                           "Se han producido %ld errores adicionales.")

/********************************************************************
        Network/server error messages
********************************************************************/

GTR_SID(SID_ERR_SHTTP_ERROR, "Error SHTTP: ")
GTR_SID(SID_ERR_COULD_NOT_INITIALIZE_NETWORK,                           "El programa no pudo inicializar la red. Cerci�rese de que la conexi�n con la red est� configurada correctamente. Aun as� podr� ver archivos locales.")
GTR_SID(SID_ERR_COULD_NOT_FIND_ADDRESS_S, "El programa no ha podido encontrar la direcci�n para '%s'.")
GTR_SID(SID_ERR_DOCUMENT_LOAD_FAILED_S, "Ha fallado el intento de carga de '%s'.")
GTR_SID(SID_ERR_NO_URL_SPECIFIED, "No se ha especificado ning�n URL.")
GTR_SID(SID_ERR_STRANGE_HTTP_SERVER_RESPONSE_S,                         "El servidor ha enviado una respuesta inesperada al programa. Como resultado '%s' no se ha cargado.")
GTR_SID(SID_ERR_SERVER_SAYS_INVALID_REQUEST_S,                          "El servidor considera la petici�n de '%s' como una solicitud no v�lida.")
GTR_SID(SID_ERR_SERVER_DENIED_ACCESS_S, "No tiene derechos de acceso a '%s'.")
GTR_SID(SID_ERR_SERVER_COULD_NOT_FIND_S, "El servidor no ha podido encontrar '%s'.")
GTR_SID(SID_ERR_NO_DESTINATION_FOR_LINK_S, "El enlace '%s' no tiene destino.")
GTR_SID(SID_ERR_INTERNAL_SERVER_ERROR_S, "Se ha producido un problema interno en el servidor. Por ello no se ha enviado '%s'.")

/********************************************************************
        Memory error messages
********************************************************************/

GTR_SID(SID_ERR_COULD_NOT_PARSE_DOCUMENT, "El programa no ha podido analizar el documento porque no hay memoria suficiente en el sistema.")
GTR_SID(SID_ERR_COULD_NOT_LOAD_IMAGE, "El programa no ha podido cargar una imagen porque no hay memoria suficiente en el sistema.")
GTR_SID(SID_ERR_COULD_NOT_PROCESS_NETWORK_RESPONSE,                     "El programa no ha podido procesar una respuesta de la red porque no hay memoria suficiente en el sistema.")
GTR_SID(SID_ERR_COULD_NOT_LOAD_DOCUMENT_IMAGES,                         "El programa no ha podido cargar las im�genes del documento porque no hay memoria suficiente en el sistema.")

/********************************************************************
        File error messages
********************************************************************/

GTR_SID(SID_ERR_COULD_NOT_SAVE_FILE_S, "No se ha podido grabar '%s'. Es posible que el disco est� lleno.")

/********************************************************************
        User error messages
********************************************************************/

GTR_SID(SID_ERR_IMAGE_MAP_NOT_LOADED_FOR_MAC,                      "Al hacer clic en esta imagen se obtienen diferentes resultados seg�n el lugar en el que pulse. Primero debe hacer clic dos veces en la imagen para cargarla.")
GTR_SID(SID_ERR_IMAGE_MAP_NOT_LOADED_FOR_WIN_UNIX,                 "Al hacer clic en esta imagen se obtienen diferentes resultados seg�n el lugar en el que pulse. Primero debe hacer clic en la imagen con el bot�n derecho del rat�n para cargarla.")
GTR_SID(SID_ERR_TEXT_NOT_FOUND_S, "No se ha encontrado el texto '%s'.")
GTR_SID(SID_ERR_HOTLIST_ALREADY_EXISTS, "Ya existe un elemento en la lista con este URL.")
GTR_SID(SID_ERR_CANNOT_MODIFY_APP_OCTET_STREAM,                         "No se admite la modificaci�n de la aplicaci�n/octect-stream")

/********************************************************************
        Miscellaneous error messages
********************************************************************/

GTR_SID(SID_ERR_COULD_NOT_COPY_TO_CLIPBOARD,                            "El programa no ha podido copiar en el portapapeles.")

/********************************************************************
        Other protocol error messages
********************************************************************/

GTR_SID(SID_ERR_NO_NEWS_SERVER_CONFIGURED, "Este programa no est� configurado para leer noticias.")
GTR_SID(SID_ERR_NO_ACCESS_TO_NEWS_SERVER_S, "No tiene derechos de acceso al servidor de noticias de '%s'.")
GTR_SID(SID_ERR_SERVER_DOES_NOT_CARRY_GROUP,                            "El servidor de noticias no incluye ese grupo.")
GTR_SID(SID_ERR_INVALID_ARTICLE_RANGE, "El grupo de art�culos seleccionado no es v�lido.")
GTR_SID(SID_ERR_NO_ARTICLES_IN_GROUP_S, "No hay art�culos en el grupo '%s'.")
GTR_SID(SID_ERR_NO_XHDR_SUPPORT, "El servidor de noticias no admite el comando XHDR.")
GTR_SID(SID_ERR_PASSIVE_MODE_NOT_SUPPORTED, "Este servidor FTP no admite el modo pasivo.")
GTR_SID(SID_ERR_PROTOCOL_NOT_SUPPORTED_S, "Este programa no admite el protocolo de acceso a '%s'.")
GTR_SID(SID_ERR_HOW_TO_RUN_TELNET_WITHOUT_USER_LOGIN_S,         "Este enclace necesita un programa telnet. Para mantener el enlace, ejecute el programa telnet y con�ctese con '%s'.")
GTR_SID(SID_ERR_HOW_TO_RUN_TELNET_WITH_USER_LOGIN_S_S,          "Este enlace necesita un progrma telnet. Para mantener el enlace, ejecute el programa telnet y con�ctese con '%s'. A continuaci�n, acceda como '%s'.")
GTR_SID(SID_ERR_HOT_TO_MAIL_S, "Este enlace necesita un programa de correo. Para mantener el enlace, ejecute dicho programa y env�e correo a '%s'.")

/********************************************************************
        Image error messages
********************************************************************/

GTR_SID(SID_ERR_INVALID_IMAGE_FORMAT, "La imagen no puede mostrarse. Puede que el formato no sea v�lido.")

/********************************************************************
        Sound error messages
********************************************************************/

GTR_SID(SID_ERR_NO_SOUND_DEVICE, "El sistema no dispone de ning�n dispositivo de sonido.")
GTR_SID(SID_ERR_NOT_ENOUGH_MEMORY_TO_PLAY_SOUND,                        "No hay suficiente memoria para reproducir el sonido.")
GTR_SID(SID_ERR_INVALID_SOUND_FORMAT, "Este archivo de sonido est� corrupto o tiene un formato desconocido.")
GTR_SID(SID_ERR_BUSY_SOUND_DEVICE, "El dispositivo de sonido se encuentra ocupado.")
GTR_SID(SID_ERR_COULD_NOT_OPEN_SOUND_FILE_S,                            "No se ha podido abrir el archivo de sonido '%s'.")

/********************************************************************
        Security error messages
********************************************************************/

GTR_SID(SID_ERR_REQUEST_ABORTED_DUE_TO_NO_ENVELOPING_1,         "El m�dulo de seguridad '%s' ha indicado que es necesario incluir esta solicitud en un sobre.  ")
GTR_SID(SID_ERR_REQUEST_ABORTED_DUE_TO_NO_ENVELOPING_2,         "Sin embargo, esta copia de %s no admite los sobres.") 
GTR_SID(SID_ERR_REQUEST_ABORTED_DUE_TO_NO_ENVELOPING_3,         "Por lo tanto, se ha interrumpido la solicitud para evitar la transmisi�n de informaci�n confidencial sin ning�n tipo de protecci�n.")

GTR_SID(SID_ERR_NO_DEENVELOPING_AVAILABLE_1,                            "El m�dulo de seguridad '%s' ha indicado que es necesario extraer este documento de un sobre.  ")
GTR_SID(SID_ERR_NO_DEENVELOPING_AVAILABLE_2, "Sin embargo, esta copia de %s no admite esta acci�n.  ")
GTR_SID(SID_ERR_NO_DEENVELOPING_AVAILABLE_3, "Por lo tanto, el documento se mostrar� tal y como se ha recibido.")

GTR_SID(SID_ERR_AUTHENTICATION_FAILED_ACCESS_DENIED,            "La autentificaci�n ha fallado. Demasiados intentos. Acceso denegado.")
GTR_SID(SID_ERR_AUTHENTICATION_REQUIRED_BUT_NOT_SPECIFIED,      "Este documento precisa autentificaci�n. El servidor no ha especificado ning�n m�todo admitido de autentificaci�n.")
GTR_SID(SID_ERR_PAYMENT_REQUIRED_BUT_NOT_SPECIFIED,                     "Es necesario pagar por este documento. El servidor no ha especificado ning�n m�todo de pago admitido.")

/********************************************************************
        MAILTO error and informational messages
********************************************************************/

GTR_SID(SID_ERR_BAD_SENDER_NAME_S, "El nombre de remitente '%s' no es v�lido.")
GTR_SID(SID_ERR_BAD_CONNECTION_S, "No se ha podido establecer conexi�n con el ordenador principal '%s'.")
GTR_SID(SID_ERR_RCPT_UNKNOWN_S, "El nombre de destinatario '%s' es desconocido.")
GTR_SID(SID_ERR_NO_MAIL_SERVER_CONFIGURED, "No hay ning�n servidor configurado para correo.")
GTR_SID(SID_ERR_BAD_SEVER_NAME_S, "El nombre de servidor '%s' no es v�lido.")
GTR_SID(SID_INF_SEND_HELLO_MESSAGE, "Se est� enviando un saludo al servidor")
GTR_SID(SID_INF_SEND_USER_NAME, "Se est� enviando el nombre de usuario al servidor")
GTR_SID(SID_INF_SEND_RCPT_NAME, "Se est� enviando acuse de recibo al servidor")
GTR_SID(SID_INF_SEND_DATA, "Se est�n enviando datos al servidor")
GTR_SID(SID_ERR_BAD_RCPT_NAME_S, "El nombre de destinatario '%s' no es v�lido.")

/********************************************************************
        SORT THESE LATER BEFORE FC
********************************************************************/

GTR_SID(SID_ERR_DCACHE_MAIN_CACHE_NO_DIR, "No se ha proporcionado un directorio de cach� principal.")
GTR_SID(SID_ERR_DCACHE_MAIN_CACHE_ERR_NEW_DIR,                          "El directorio de cach� principal no existe y se ha producido un error al crearlo.")
GTR_SID(SID_DCACHE_MAIN_CACHE_CREATED_NEW_DIR_S,                        "El directorio de cach� principal no exist�a y se ha creado en %s.")
GTR_SID(SID_ERR_DCACHE_MAIN_CACHE_ERR_BAD_DIR,                          "El directorio de cach� principal no es v�lido (no es un directorio o no dispone de suficientes derechos de acceso a �l).")

GTR_SID(SID_INF_GOPHER_ENTER_KEYWORDS, "\nEn este �ndice gopher se pueden realizar b�squedas. Escriba las palabras clave que desee buscar.\n")
GTR_SID(SID_INF_CSO_ENTER_KEYWORDS, "\nEn este �ndice de una base de datos CSO se pueden realizar b�squedas.\nEscriba las palabras clave que desee buscar. Las palabras que utilice le permitir�n buscar el nombre de una persona en la base de datos.\n")
GTR_SID(SID_INF_GOPHER_SELECT_ONE_OF, "Seleccione uno de:\n\n")
GTR_SID(SID_INF_CSO_SEARCH_RESULTS, "Resultados de la b�squeda CSO")


