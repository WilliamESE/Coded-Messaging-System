switch(State)
	{
	case 0: //Get header
		Longer = 0;
		for(x=0;x<4 && x<cnt;x++)
			Longer = (Longer * 1000) + (int)Qu[x];
		if (Longer == 0xDEADBEEF)
			State=1;		
		if (cnt > 4)
			cnt = 0;
		break;
	case 1: //Get receiver ID
		if (cnt >= 1)
			{
			Mrec = Qu[0];
			State = 2;
			}
		cnt = 0;
		break;
	case 2: //Get Header type
		if (cnt >= 1)
			if (Qu[0] == 1)
				State = 3;
			else if(Qu[0] == 0)
				State = 6;
			else
				State = 0;
		cnt = 0;
		break;
	case 3: //Version
		if (cnt >= 1)
			if (Qu[0] == 1)
				State = 4;
			else
				State = 0;
		cnt = 0;
		break;
	case 4: //Data Length
		if (cnt >= 1)
			Mlen = Qu[0];
		State=5;
		cnt = 0;
		break;
	case 5: //Header End
		if ((Qu[0] == 0xAA) && (Qu[1] == 0x55) && (Qu[2] == 0xAA) && (Qu[3] == 0x55))
			State = 8;
		if (cnt > 4)
			cnt = 0;
		break;
	case 6: //Type
		if (cnt >= 1)
			{
			Mtyp = Qu[0];
			State = 7;
			}
		cnt = 0;
		break;
	case 7: //Message Length
		if ((cnt >= 1) && (Mtyp == 1))
			{
			Mlen = Qu[0];
			State=8;
			cnt = 0;
			}
		else if((cnt > 4) && (Mtyp == 0))
			{
			Longer = 0;
			for(x=0;x<4 && x<cnt;x++)
				Longer = (Longer * 1000) + (int)Qu[x];	
			Rlen = Longer;
			State = 12;
			cnt = 0;
			}
		break;
	case 8: //Message
		if (cnt >= Mlen)
			{
			for(x=0;x<Mlen;x++)
				hout[x] = Qu[x];
			cnt=0;
			State=9;
			}
		break;
	case 9: //Sender
		if (cnt >= 1)
			{
			Msen = Qu[0];
			State = 10;
			}
		cnt = 0;
		break;
	case 10: //Priority
		if (cnt >= 1)
			{
			Mpri = Qu[0];
			State = 11;
			}
		cnt = 0;
		break;
	case 11: //Extra
		if (cnt >= 1)
			{
			if(Qu[0] == 0)
				{
				///Add to queue	
				}
			else
				{
				///Call functions depending on the type
				}
			State = 0;
			}
		cnt = 0;
		break;
	case 12: //Compress type
		if (cnt >= 1)
			{
			Rcom = Qu[0];
			State = 13;
			}
		cnt = 0;
		break;
	case 13: //Record Data
		hout[cnt] = Qu[cnt];
		if (cnt >= Len)
			{
			cnt=0;
			///Call decompress functions and stuff
			State = 0;
			}
		break;
	}
