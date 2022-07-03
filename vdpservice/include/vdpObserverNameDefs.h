/* ********************************************************************************* *
 * Copyright (C) 2014-2021 VMware, Inc.  All rights reserved. -- VMware Confidential *
 * ********************************************************************************* */

/*
 * vdpObserverNameDefs.h --
 *
 *    This header file defines the names registered to
 *    VDPService_ObserverInterface used by components which try to communicate
 *    through the interface.
 */


// Observer name for sending folder redirection command from remotemks to plugin
#define FOLDER_REDIRECTION_CMD "FORLDER_REDIRECTION_CMD"

// Observer name for sending folder redirection notification from plugin to remotemks
#define FOLDER_REDIRECTION_NOTIFICATION "FOLDER_REDIRECTION_NOTIFICATION"

// Observer name for sending Geolocation redirection command from remotemks to plugin
#define GEOLOCATION_REDIRECTION_CMD "GEOLOCATION_REDIRECTION_CMD"

// Observer name for sending Geolocation redirection notification from plugin to remotemks
#define GEOLOCATION_REDIRECTION_NOTIFICATION "GEOLOCATION_REDIRECTION_NOTIFICATION"

// Observer name for sending rde common command from remotemks to plugin
#define RDE_COMMON_GENERIC_CMD "RDE_COMMON_GENERIC_CMD"

// Observer name for sending rde common command from plugin to remotemks
#define RDE_COMMON_GENERIC_NOTIFICATION "RDE_COMMON_GENERIC_NOTIFICATION"

// Observer name for sending html5 messages between plugin and remotemks
#define HTML5_REDIRECTION_MSG "HTML5_REDIRECTION_MSG"
