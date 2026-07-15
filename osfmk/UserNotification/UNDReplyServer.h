#ifndef _USERNOTIFICATION_UND_REPLY_SERVER_H_
#define _USERNOTIFICATION_UND_REPLY_SERVER_H_

#include <mach/mig.h>
#include <UserNotification/UNDTypes.h>

extern const struct mig_subsystem UNDReply_subsystem;

kern_return_t UNDAlertCompletedWithResult_rpc(UNDReplyRef reply, int result,
    xmlData_t data, mach_msg_type_number_t dataCnt);
kern_return_t UNDNotificationCreated_rpc(UNDReplyRef reply,
    int userLandNotificationKey);

#endif /* _USERNOTIFICATION_UND_REPLY_SERVER_H_ */
