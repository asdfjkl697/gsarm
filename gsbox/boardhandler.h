#ifndef _BOARD_HANDLER_H
#define _BOARD_HANDLER_H

class BoardHandler
{
	public:
		virtual int Send(uint8_t *data, int size) = 0;
		virtual uint8_t *GetVersion() = 0;
};

#endif
