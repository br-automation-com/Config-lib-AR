
TYPE
	cfg_internal_typ : 	STRUCT  (*Internal structure*)
		y : UINT;
		repeat : UINT;
		step : CFG_STEP;
		offset : UINT;
		file_size : UINT;
		buffer : ARRAY[0..BUFFER_SIZE]OF USINT;
		status : UINT;
		FOpen : FileOpen;
		FRead : FileRead;
		FWrite : FileWrite;
		FCreate : FileCreate;
		FDelete : FileDelete;
		FCopy : FileCopy;
		FClose : FileClose;
	END_STRUCT;
	cfg_typ : 	STRUCT  (*Main function structure*)
		read : BOOL; (*Read a configuration entry*)
		write : BOOL; (*Write a configuration entry*)
		device : UDINT; (*File device name as pointer*)
		file_name : UDINT; (*File name as pointer*)
		entry : STRING[ENTRY_SIZE]; (*Name of the entry as string*)
		data : STRING[DATA_SIZE]; (*Entry data as string*)
		option : USINT; (*On write command create new entry if it does not exist*)
		internal : cfg_internal_typ; (*Internal data structure*)
	END_STRUCT;
	CFG_STEP : 
		( (*State machine*)
		CFG_STP_WAIT,
		CFG_STP_OPEN,
		CFG_STP_READ,
		CFG_STP_WRITE,
		CFG_STP_COPY,
		CFG_STP_CREATE,
		CFG_STP_FIND,
		CFG_STP_DELETE,
		CFG_STP_CLOSE
		);
END_TYPE
