/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DUMP_MESSAGE_H
#define _DUMP_MESSAGE_H

class BMessage;
class BDataIO;

status_t DumpMessageToStream(BMessage *message, BDataIO &stream, int tabCount = 0, BMessage *names = NULL);

#endif /* _DUMP_MESSAGE_H */

