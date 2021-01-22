PROGRAM _INIT
	// Set read and write config parameters
	cfg_0.device	:= ADR('UserData');
	cfg_0.file_name	:= ADR('config.txt');
END_PROGRAM

PROGRAM _CYCLIC
	CASE step OF
		// --------------------------------------------------------------------------------------------------
		// Wait for new command
		STEP_WAIT:
			// Command read all configuration data
			IF cmdReadAll THEN
				read := 0;
				step := STEP_READ;
			END_IF
			// Command single read or write data
			IF cmdWrite OR cmdRead THEN
				cfg_0.read := cmdRead;
				cfg_0.write := cmdWrite;
				step := STEP_EXECUTE;
			END_IF
		// --------------------------------------------------------------------------------------------------
		// Read complete configuration
		STEP_READ:
			CASE read OF
				// Read password
				0:	
					cfg_0.entry := 'Password';
					cfg_0.read := TRUE;
					step := STEP_EXECUTE;
				// Set password, read max speed
				1:
					Password := cfg_0.data;
					cfg_0.entry := 'MaxSpeed';
					cfg_0.read := TRUE;
					step := STEP_EXECUTE;
				// Set max speed, read temperature
				2:
					MaxSpeed := DINT_TO_UINT(brsatoi(ADR(cfg_0.data)));
					cfg_0.entry := 'MinTemperature';
					cfg_0.read := TRUE;
					step := STEP_EXECUTE;
				// Set temperature, finish up
				3:
					MinTemperature := brsatof(ADR(cfg_0.data));
					cmdReadAll := FALSE;
					step := STEP_WAIT;
			END_CASE
		// --------------------------------------------------------------------------------------------------
		// Call function for read and write configuration
		STEP_EXECUTE:
			status := fct_cfg(cfg_0);
			IF status <> ERR_FUB_BUSY THEN
				// Return to read all state machine
				IF cmdReadAll THEN
					read := read + 1;
					step := STEP_READ;
				// Finish single read and write command
				ELSE
					cmdRead := FALSE;
					cmdWrite := FALSE;
					step := STEP_WAIT;
				END_IF
			END_IF
	END_CASE
END_PROGRAM

