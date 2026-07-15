#ifndef _USERNOTIFICATION_UND_REQUEST_H_
#define _USERNOTIFICATION_UND_REQUEST_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <UserNotification/UNDTypes.h>

kern_return_t UNDExecute_rpc(UNDServerRef server, UNDPath executionPath,
    int uid, int gid);
kern_return_t UNDDisplayNoticeFromBundle_rpc(UNDServerRef server,
    mach_port_t reply, UNDPath bundlePath, UNDPath fileName,
    UNDPath fileExtension, UNDKey messageKey);
kern_return_t UNDDisplayAlertFromBundle_rpc(UNDServerRef server,
    mach_port_t reply, UNDPath bundlePath, UNDKey fileName,
    UNDKey fileExtension, UNDKey messageKey);
kern_return_t UNDDisplayCustomFromBundle_rpc(UNDServerRef server,
    mach_port_t reply, UNDPath bundlePath, UNDKey fileName,
    UNDKey fileExtension, UNDKey messageKey, UNDPath tokenKey);
kern_return_t UNDCancelNotification_rpc(UNDServerRef server,
    int userLandNotificationKey);
kern_return_t UNDDisplayNoticeSimple_rpc(UNDServerRef server,
    int rpctimeout, unsigned flags, UNDLabel iconPath, UNDLabel soundPath,
    UNDLabel localizationPath, UNDLabel header, UNDMessage message,
    UNDLabel defaultButtonTitle);
kern_return_t UNDDisplayAlertSimple_rpc(UNDServerRef server,
    int rpctimeout, unsigned flags, UNDLabel iconPath, UNDLabel soundPath,
    UNDLabel localizationPath, UNDLabel header, UNDMessage message,
    UNDLabel defaultButtonTitle, UNDLabel alternateButtonTitle,
    UNDLabel otherButtonTitle, unsigned *response);

#endif /* _USERNOTIFICATION_UND_REQUEST_H_ */
