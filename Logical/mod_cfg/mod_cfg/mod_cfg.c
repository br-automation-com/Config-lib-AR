
#include <bur/plctypes.h>
#ifdef __cplusplus
	extern "C"
	{
#endif
	#include "mod_cfg.h"
#ifdef __cplusplus
	};
#endif

// ------------------------------------------------------------------------------------------------
// Local functions
unsigned short create_error(cfg_typ* inst, UINT code, UINT step);

/************************************************************************************************************/
/*	Write configuration																						*/
/************************************************************************************************************/
unsigned short fct_cfg(cfg_typ* inst)
{
	for(inst->internal.repeat=0;inst->internal.repeat<OVERRIDE;inst->internal.repeat++)
	{
		switch(inst->internal.step)
		{
			/****************************************************************************************************/
			/* Wait state																						*/
			/****************************************************************************************************/
			case CFG_STP_WAIT:
				// ------------------------------------------------------------------------------------------------
				// Wait for start command
				if(inst->read || inst->write)
				{
					// ------------------------------------------------------------------------------------------------
					// Verify that devie isn't empty
					if(inst->device == 0 || brsstrlen(inst->device) == 0)
					{
						return create_error((cfg_typ*)inst, ERR_ENTRY_EMPTY, CFG_STP_WAIT);
					}
					// ------------------------------------------------------------------------------------------------
					// Verify that entry isn't empty
					if(inst->entry == 0 || brsstrlen((UDINT)inst->entry) == 0)
					{
						return create_error((cfg_typ*)inst, ERR_DEVICE_EMPTY, CFG_STP_WAIT);
					}
					brsmemset((UDINT)&inst->internal.buffer, 0, sizeof(inst->internal.buffer));
					inst->internal.offset = 0;
					inst->internal.status = ERR_OK;
					inst->internal.step = CFG_STP_OPEN;
					return ERR_FUB_BUSY;
				}
				break;
			
			/****************************************************************************************************/
			/* Open file																						*/
			/****************************************************************************************************/
			case CFG_STP_OPEN:
				inst->internal.FOpen.enable	 	= 1;
				inst->internal.FOpen.pDevice 	= inst->device;
				inst->internal.FOpen.pFile 		= (UDINT)inst->file_name;
				inst->internal.FOpen.mode 		= FILE_RW; 
				FileOpen(&inst->internal.FOpen);
				
				// ------------------------------------------------------------------------------------------------
				// Success
				if (inst->internal.FOpen.status == ERR_OK)
				{
					// ------------------------------------------------------------------------------------------------
					// Verify that configuration file isnt't too long
					if(inst->internal.FOpen.filelen > BUFFER_SIZE)
					{
						inst->internal.step = CFG_STP_CLOSE;
						return ERR_FILE_SIZE;
					}
					inst->internal.offset = 0;
					inst->internal.step = CFG_STP_READ;				
				}
				// ------------------------------------------------------------------------------------------------
				// File dont exist
				else if (inst->internal.FOpen.status == fiERR_FILE_NOT_FOUND)
				{
					inst->internal.FOpen.filelen = 0;
					inst->internal.step = CFG_STP_CREATE;
				}
				// ------------------------------------------------------------------------------------------------
				// Failure
				else if (inst->internal.FOpen.status != ERR_FUB_BUSY)
				{
					return create_error((cfg_typ*)inst, inst->internal.FOpen.status, CFG_STP_WAIT);
				}		
				break;
			
			/****************************************************************************************************/
			/* Read file																						*/
			/****************************************************************************************************/
			case CFG_STP_READ:
				inst->internal.FRead.enable	= 1;
				inst->internal.FRead.ident	= inst->internal.FOpen.ident;
				inst->internal.FRead.offset	= 0;
				inst->internal.FRead.pDest	= (UDINT)&inst->internal.buffer;
				inst->internal.FRead.len	= inst->internal.FOpen.filelen;
				FileRead(&inst->internal.FRead);
				
				// ------------------------------------------------------------------------------------------------
				// Success
				if (inst->internal.FRead.status == ERR_OK)
				{
					inst->internal.step = CFG_STP_CLOSE;
				}
				// ------------------------------------------------------------------------------------------------
				// Failure
				else if (inst->internal.FRead.status != ERR_FUB_BUSY)
				{
					return create_error((cfg_typ*)inst, inst->internal.FRead.status, CFG_STP_WAIT);
				}		
				break;
			
			/****************************************************************************************************/
			/* Delete old file																					*/
			/****************************************************************************************************/
			case CFG_STP_DELETE:
				inst->internal.FDelete.enable	= 1;
				inst->internal.FDelete.pDevice	= inst->device;
				inst->internal.FDelete.pName 	= (UDINT)TMP_FILE;
				FileDelete(&inst->internal.FDelete);
				
				// ------------------------------------------------------------------------------------------------
				// Success
				if (inst->internal.FDelete.status == ERR_OK || inst->internal.FDelete.status == fiERR_FILE_NOT_FOUND)
				{
					inst->internal.step = CFG_STP_CREATE;
				}
				// ------------------------------------------------------------------------------------------------
				// Failure
				else if (inst->internal.FDelete.status != ERR_FUB_BUSY)
				{
					return create_error((cfg_typ*)inst, inst->internal.FDelete.status, CFG_STP_WAIT);
				}		
				break;
			/****************************************************************************************************/
			/* Create file																						*/
			/****************************************************************************************************/
			case CFG_STP_CREATE:
				inst->internal.FCreate.enable	= 1;
				inst->internal.FCreate.pDevice	= inst->device;
				inst->internal.FCreate.pFile 	= (UDINT)"tmp";
				FileCreate(&inst->internal.FCreate);
				
				// ------------------------------------------------------------------------------------------------
				// Success
				if (inst->internal.FCreate.status == ERR_OK)
				{
					inst->internal.step = CFG_STP_WRITE;
				}
				// ------------------------------------------------------------------------------------------------
				// Failure
				else if (inst->internal.FCreate.status != ERR_FUB_BUSY)
				{
					return create_error((cfg_typ*)inst, inst->internal.FCreate.status, CFG_STP_WAIT);
				}		
				break;
			/****************************************************************************************************/
			/* Write file																						*/
			/****************************************************************************************************/
			case CFG_STP_WRITE:
				inst->internal.FWrite.enable	= 1;
				inst->internal.FWrite.ident		= inst->internal.FCreate.ident;
				inst->internal.FWrite.offset	= 0;
				inst->internal.FWrite.pSrc		= (UDINT)&inst->internal.buffer;
				inst->internal.FWrite.len		= inst->internal.file_size;
				FileWrite(&inst->internal.FWrite);
				
				// ------------------------------------------------------------------------------------------------
				// Success
				if  (inst->internal.FWrite.status == ERR_OK)
				{
					inst->internal.step = CFG_STP_CLOSE;
				}
				// Failure
				else if (inst->internal.FWrite.status != ERR_FUB_BUSY)
				{
					inst->internal.status = inst->internal.FWrite.status;
					inst->internal.step = CFG_STP_CLOSE;
				}		
				break;
			/****************************************************************************************************/
			/* Find entry in data																				*/
			/****************************************************************************************************/
			case CFG_STP_FIND:
				// ------------------------------------------------------------------------------------------------
				// Found entry
				if((brsmemcmp((UDINT)((UDINT)&inst->internal.buffer+inst->internal.offset), (UDINT)inst->entry, brsstrlen((UDINT)inst->entry)) == 0) &&
					(brsmemcmp((UDINT)((UDINT)&inst->internal.buffer+inst->internal.offset+brsstrlen((UDINT)inst->entry)), (UDINT)EQUAL, 1) == 0))
				{
					// ------------------------------------------------------------------------------------------------
					// Find the end of line
					inst->internal.y = 0;
					for(inst->internal.y=brsstrlen((UDINT)inst->entry)+1;inst->internal.y<inst->internal.FOpen.filelen-inst->internal.offset;inst->internal.y++)
					{
						if(inst->internal.buffer[inst->internal.y+inst->internal.offset] == LINE_FEED || inst->internal.buffer[inst->internal.y+inst->internal.offset] == CARRIAGE_RETURN) break;
					}
					// ------------------------------------------------------------------------------------------------
					// Command is read
					if (inst->read)
					{
						// Make sure data fits into variable
						if(inst->internal.y-brsstrlen((UDINT)inst->entry)-1<DATA_SIZE)
						{
							// Reset data point and copy data
							brsmemset((UDINT)&inst->data, 0, sizeof(inst->data));
							brsmemcpy((UDINT)&inst->data, (UDINT)((UDINT)&inst->internal.buffer+inst->internal.offset+brsstrlen((UDINT)inst->entry)+1), inst->internal.y-brsstrlen((UDINT)inst->entry)-1);
							inst->read = 0;
							inst->internal.step = CFG_STP_WAIT;
							return ERR_OK;
						}
						// New data pushes buffer over the edge 
						else
						{
							return create_error((cfg_typ*)inst, ERR_DATA_SIZE, CFG_STP_WAIT);
						}
					}
					// ------------------------------------------------------------------------------------------------
					// Command is write
					else
					{
						// Make sure entry still fits
						if(inst->internal.y-brsstrlen((UDINT)inst->entry)+1+brsstrlen((UDINT)inst->data)+inst->internal.FOpen.filelen<BUFFER_SIZE)
						{
							brsmemmove((UDINT)((UDINT)&inst->internal.buffer+inst->internal.offset+brsstrlen((UDINT)inst->entry)+brsstrlen((UDINT)inst->data))+1, (UDINT)((UDINT)&inst->internal.buffer+inst->internal.offset+inst->internal.y), inst->internal.FOpen.filelen-inst->internal.offset-inst->internal.y);
						}
						// New data pushes buffer over the edge 
						else
						{
							return create_error((cfg_typ*)inst, ERR_BUFFER_SIZE, CFG_STP_WAIT);
						}
						// New entry is shorter than before, erase tailing bytes
						if(brsstrlen((UDINT)inst->entry)+1+brsstrlen((UDINT)inst->data)<inst->internal.y) 
						{
							brsmemset((UDINT)((UDINT)&inst->internal.buffer+inst->internal.FOpen.filelen-(inst->internal.y-(brsstrlen((UDINT)inst->entry)+1+brsstrlen((UDINT)inst->data)))), 0, inst->internal.y-(brsstrlen((UDINT)inst->entry)+1+brsstrlen((UDINT)inst->data)));
						}
						brsmemcpy((UDINT)((UDINT)&inst->internal.buffer+inst->internal.offset+brsstrlen((UDINT)inst->entry)+1), (UDINT)inst->data, brsstrlen((UDINT)inst->data));
						inst->internal.file_size	= inst->internal.offset+brsstrlen((UDINT)inst->entry)+brsstrlen((UDINT)inst->data)+(inst->internal.FOpen.filelen-inst->internal.offset-inst->internal.y)+1;
						inst->internal.step 		= CFG_STP_DELETE;
						return ERR_FUB_BUSY;
					}
				}
				// ------------------------------------------------------------------------------------------------
				// Search for end of line
				else
				{
					for(inst->internal.y=inst->internal.offset;inst->internal.y<inst->internal.FOpen.filelen;inst->internal.y++)
					{
						if(inst->internal.buffer[inst->internal.y] == LINE_FEED || inst->internal.buffer[inst->internal.y] == CARRIAGE_RETURN)
						{
							inst->internal.offset = inst->internal.y + 1;
							return ERR_FUB_BUSY;
						}
					}
					// ------------------------------------------------------------------------------------------------
					// Create entry if it dont exists
					if(inst->option == CFG_CREATE)
					{
						// Make sure new entry fits at the end
						if(inst->internal.y+brsstrlen((UDINT)inst->entry)+brsstrlen((UDINT)inst->data)+1)
						{
							// Add line feed at the end if it does not exist
							if(inst->internal.y > 0 && inst->internal.buffer[inst->internal.y-1] != LINE_FEED && inst->internal.buffer[inst->internal.y-1] != CARRIAGE_RETURN)
							{
								inst->internal.buffer[inst->internal.y] = LINE_FEED;
								inst->internal.y++;
							}
							brsmemcpy((UDINT)((UDINT)&inst->internal.buffer+inst->internal.y), (UDINT)inst->entry, brsstrlen((UDINT)inst->entry));
							brsmemcpy((UDINT)((UDINT)&inst->internal.buffer+inst->internal.y+brsstrlen((UDINT)inst->entry)), (UDINT)EQUAL, 1);
							brsmemcpy((UDINT)((UDINT)&inst->internal.buffer+inst->internal.y+brsstrlen((UDINT)inst->entry)+1), (UDINT)inst->data, brsstrlen((UDINT)inst->data));
							inst->internal.file_size = inst->internal.y+brsstrlen((UDINT)inst->entry)+1+brsstrlen((UDINT)inst->data);
							inst->internal.step = CFG_STP_DELETE;
						}
						// New data pushes buffer over the edge 
						else
						{
							return create_error((cfg_typ*)inst, ERR_BUFFER_SIZE, CFG_STP_WAIT);
						}
					}
					// ------------------------------------------------------------------------------------------------
					// Create error if entry does not exist
					else
					{
						return create_error((cfg_typ*)inst, ERR_ENTRY_NOT_FOUND, CFG_STP_WAIT);
					}
				}
				break;
			/****************************************************************************************************/
			/* Close file																						*/
			/****************************************************************************************************/
			case CFG_STP_CLOSE:
				inst->internal.FClose.enable = 1;
				// Close file from reading
				if (inst->internal.FOpen.ident != 0)
				{
					inst->internal.FClose.ident	= inst->internal.FOpen.ident;
				}
				// Close file from writing
				else
				{
					inst->internal.FClose.ident	= inst->internal.FCreate.ident;
				}
				FileClose(&inst->internal.FClose);
				
				// ------------------------------------------------------------------------------------------------
				// Success
				if (inst->internal.FClose.status == ERR_OK)
				{
					// First run from command write or read
					if (inst->internal.FOpen.ident != 0)
					{
						inst->internal.FOpen.ident = 0;
						inst->internal.step = CFG_STP_FIND;
					}
					// Second run from command write
					else
					{
						// Report error from previous step
						if (inst->internal.status != 0)
						{
							return create_error((cfg_typ*)inst, inst->internal.status, CFG_STP_WAIT);
						}
						inst->internal.step = CFG_STP_COPY;
					}
				}
				// ------------------------------------------------------------------------------------------------
				// Failure
				else if (inst->internal.FClose.status != ERR_FUB_BUSY)
				{
					// Report error from previous step
					if (inst->internal.status != 0)
					{
						return create_error((cfg_typ*)inst, inst->internal.status, CFG_STP_WAIT);
					}
					return create_error((cfg_typ*)inst, inst->internal.FClose.status, CFG_STP_WAIT);
				}		
				break;
			/****************************************************************************************************/
			/* Copy file																						*/
			/****************************************************************************************************/
			case CFG_STP_COPY:
				inst->internal.FCopy.enable		= 1;
				inst->internal.FCopy.pSrcDev	= inst->device;
				inst->internal.FCopy.pSrc		= (UDINT)"tmp";
				inst->internal.FCopy.pDestDev	= inst->device;
				inst->internal.FCopy.pDest		= (UDINT)inst->file_name;
				inst->internal.FCopy.option		= fiOVERWRITE ;
				FileCopy(&inst->internal.FCopy);
				
				// ------------------------------------------------------------------------------------------------
				// Success
				if (inst->internal.FCopy.status == ERR_OK)
				{				
					inst->write = 0;
					inst->internal.step = CFG_STP_WAIT;
					return ERR_OK;
				}
				// ------------------------------------------------------------------------------------------------
				// Failure
				else if (inst->internal.FCopy.status != ERR_FUB_BUSY)
				{
					return create_error((cfg_typ*)inst, inst->internal.FCopy.status, CFG_STP_WAIT);
				}		
				break;
		}
	}
	return ERR_FUB_BUSY;
}

// ------------------------------------------------------------------------------------------------
// Reset execute and return error
// ------------------------------------------------------------------------------------------------
unsigned short create_error(cfg_typ* inst, UINT code, UINT step)
{
	inst->read = 0;
	inst->write = 0;
	inst->internal.status = 0;
	inst->internal.step = step;
	return code;
}



