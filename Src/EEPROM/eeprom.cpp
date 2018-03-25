
#include "eeprom.hpp"





#ifdef  _EEPROM_F1_LOW_DESTINY
#define		_EEPROM_FLASH_PAGE_SIZE								1024
/* Base address of the Flash sectors */
#define ADDR_FLASH_PAGE_0     ((uint32_t)0x08000000) /* Base @ of Page 0, 1 Kbytes */
#define _EEPROM_FLASH_PAGE_ADDRESS    (ADDR_FLASH_PAGE_0|(_EEPROM_FLASH_PAGE_SIZE*_EEPROM_USE_FLASH_PAGE))
#if (_EEPROM_USE_FLASH_PAGE>31)
#error  "Please Enter correct value _EEPROM_USE_FLASH_PAGE  (0 to 31)"
#endif
#endif


#ifdef  _EEPROM_F1_MEDIUM_DESTINY
#define		_EEPROM_FLASH_PAGE_SIZE								1024
/* Base address of the Flash sectors */
#define ADDR_FLASH_PAGE_0     ((uint32_t)0x08000000) /* Base @ of Page 0, 1 Kbytes */
#define _EEPROM_FLASH_PAGE_ADDRESS    (ADDR_FLASH_PAGE_0|(_EEPROM_FLASH_PAGE_SIZE*_EEPROM_USE_FLASH_PAGE))
#if (_EEPROM_USE_FLASH_PAGE>127)
#error  "Please Enter correct value _EEPROM_USE_FLASH_PAGE  (0 to 127)"
#endif
#endif


#ifdef  _EEPROM_F1_HIGH_DESTINY
#define		_EEPROM_FLASH_PAGE_SIZE								2048
/* Base address of the Flash sectors */
#define ADDR_FLASH_PAGE_0     ((uint32_t)0x08000000) /* Base @ of Page 0, 2 Kbytes */
#define _EEPROM_FLASH_PAGE_ADDRESS    (ADDR_FLASH_PAGE_0|(_EEPROM_FLASH_PAGE_SIZE*_EEPROM_USE_FLASH_PAGE))
#if (_EEPROM_USE_FLASH_PAGE>255)
#error  "Please Enter correct value _EEPROM_USE_FLASH_PAGE  (0 to 255)"
#endif
#endif


#ifdef INDIVIDUAL_ADDR_WRITE
uint8_t	EEPROMPageBackup[_EEPROM_FLASH_PAGE_SIZE];
#endif


//##########################################################################################################
//##########################################################################################################
//##########################################################################################################


bool    EE_LL_Format(uint16_t startpage, uint16_t pages)
{
  uint32_t    error;
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef  flashErase;
    flashErase.NbPages=pages;
    flashErase.Banks = FLASH_BANK_1;
    flashErase.PageAddress = (ADDR_FLASH_PAGE_0|(_EEPROM_FLASH_PAGE_SIZE*startpage));
    flashErase.TypeErase = FLASH_TYPEERASE_PAGES;
    if(HAL_FLASHEx_Erase(&flashErase,&error)==HAL_OK)
    {
        HAL_FLASH_Lock();
        if(error != 0xFFFFFFFF)
            return false;
        else
            return true;
    }
    HAL_FLASH_Lock();
    return false;
}

bool    EE_LL_Read(uint16_t startpage, uint16_t addr, uint16_t size, uint8_t* Data)
{
    for(uint16_t ii=addr;ii<size+addr;ii++)
    {
        *Data =  (*(__IO uint8_t*)(ii+(ADDR_FLASH_PAGE_0|(_EEPROM_FLASH_PAGE_SIZE*startpage))));
        Data++;
    }
    return true;
}


#ifdef INDIVIDUAL_ADDR_WRITE

//write [Size] bytes starting at [Addr] with a page-offset of [startpage]


bool    EE_LL_Write(uint16_t startpage, uint16_t addr, uint16_t size, uint8_t* Data)
{
  uint16_t jj;
  uint16_t pages_to_write = size/_EEPROM_FLASH_PAGE_SIZE;
  uint16_t reminder_to_write = size % _EEPROM_FLASH_PAGE_SIZE;
  uint16_t skippage =  addr/_EEPROM_FLASH_PAGE_SIZE;
  //Find first page we need to modify

  startpage = startpage + skippage;
  addr = addr - (skippage *_EEPROM_FLASH_PAGE_SIZE);
  if (reminder_to_write > 0)
    pages_to_write++;


  //Loop though chunks of pages
  for (uint8_t ii = startpage;ii<startpage+pages_to_write;ii++)
  {
    //Create backup
    if( EE_LL_Read(ii,0,_EEPROM_FLASH_PAGE_SIZE,(uint8_t*)EEPROMPageBackup)==false)
        return false;

  //First format page
    if(EE_LL_Format(ii,1)==false)
        return false;

    //Calculate how many bytes to write in this chunk
    uint16_t this_write = size;
    if (this_write > 1024)
      this_write = 1024;

    for(jj=addr ; jj<addr + this_write ; jj++)
    {
        EEPROMPageBackup[jj]=*Data;
        Data++;
    }

    //Store data
    HAL_FLASH_Unlock();
    for(uint16_t jj=0 ; jj<(_EEPROM_FLASH_PAGE_SIZE/4); jj++)
    {
      if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,jj+(ADDR_FLASH_PAGE_0|(_EEPROM_FLASH_PAGE_SIZE*ii)),(uint64_t)EEPROMPageBackup[jj])!=HAL_OK)
      {
        HAL_FLASH_Lock();
        return false;
      }
    }
    HAL_FLASH_Lock();

    //If we write more then one page we will from now on always start at the beginning of the page...
    addr = 0;
    //Reduce size by number of bytes written
    size = size - this_write;
  }
  return true;
}
#else

//write [Size] bytes starting at [Addr] with a page-offset of [startpage]
//Addr has to be dividable by 1024 and size a multiplier of 1024...
bool    EE_LL_Write(uint16_t startpage, uint16_t addr, uint16_t size, uint8_t* Data)
{


  if (addr % _EEPROM_FLASH_PAGE_SIZE)
    return false;
  if (size % _EEPROM_FLASH_PAGE_SIZE)
    return false;

  uint16_t skippage =  addr/_EEPROM_FLASH_PAGE_SIZE;

  //Find first page we need to modify
  startpage = startpage + skippage;


  //Loop though chunks of pages
  for (uint8_t ii = startpage;ii<startpage+(size / _EEPROM_FLASH_PAGE_SIZE);ii++)
  {
    //First format page
    if(EE_LL_Format(ii,1)==false)
        return false;


    //Store data
    HAL_FLASH_Unlock();
    for(uint16_t jj=0 ; jj<(_EEPROM_FLASH_PAGE_SIZE/4); jj++)
    {
    	uint64_t pdata = ((uint64_t)Data[1] << 24) + ((uint64_t)Data[0] << 16) + ((uint64_t)Data[3]<<8) + ((uint64_t)Data[2]);
    	uint32_t waddr = (jj*4)+(ADDR_FLASH_PAGE_0|(_EEPROM_FLASH_PAGE_SIZE*ii));
      if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,waddr,pdata)!=HAL_OK)
      {
        HAL_FLASH_Lock();
        return false;
      }
      Data = Data + 4;
    }
    HAL_FLASH_Lock();
  }
  return true;
}
#endif


//##########################################################################################################
//##########################################################################################################

bool	EE_Format()
{
  return EE_LL_Format(_EEPROM_USE_FLASH_PAGE,1);
}

//##########################################################################################################
bool EE_Read(uint16_t VirtualAddress, uint8_t* Data)
{
  return EE_LL_Read(_EEPROM_USE_FLASH_PAGE, VirtualAddress, 1, Data);
}

//##########################################################################################################
bool EE_Write(uint16_t VirtualAddress, uint8_t Data)
{
  return EE_LL_Write(_EEPROM_USE_FLASH_PAGE, VirtualAddress, 1, &Data);
}

//##########################################################################################################
bool EE_Reads(uint16_t VirtualAddress,uint16_t HowMuchToRead,uint32_t* Data)
{
	if((VirtualAddress+HowMuchToRead) >	(_EEPROM_FLASH_PAGE_SIZE/4))
		return false;
	for(uint16_t	i=VirtualAddress ; i<HowMuchToRead+VirtualAddress ; i++)
	{
		*Data =  (*(__IO uint32_t*)((i*4)+_EEPROM_FLASH_PAGE_ADDRESS));
		Data++;
	}
	return true;
}

//##########################################################################################################
bool 	EE_Writes(uint16_t VirtualAddress,uint16_t HowMuchToWrite,uint32_t* Data)
{
	if((VirtualAddress+HowMuchToWrite) >	(_EEPROM_FLASH_PAGE_SIZE/4))
		return false;
	/*
	if( EE_Reads(0,(_EEPROM_FLASH_PAGE_SIZE/4),EEPROMPageBackup)==false)
		return false;
	for(uint16_t	i=StartVirtualAddress ; i<HowMuchToWrite+StartVirtualAddress ; i++)
	{
		EEPROMPageBackup[i]=*Data;
		Data++;
	}
	*/
	if(EE_Format()==false)
		return false;
	HAL_FLASH_Unlock();
	for(uint16_t	i=0 ; i<(_EEPROM_FLASH_PAGE_SIZE/4); i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,(i*4)+_EEPROM_FLASH_PAGE_ADDRESS,(uint64_t)*Data)!=HAL_OK)
		{
			HAL_FLASH_Lock();
			return false;
		}
		Data++;
	}
	HAL_FLASH_Lock();
	return true;
}
//##########################################################################################################
