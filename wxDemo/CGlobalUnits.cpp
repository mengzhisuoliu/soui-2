#include "stdafx.h"
#include "CGlobalUnits.h"
#include <sstream>

CGlobalUnits::CGlobalUnits(void)
{
	GenerateShamDate();
}

CGlobalUnits::~CGlobalUnits(void)
{
}


CGlobalUnits* CGlobalUnits::instance()
{
	static CGlobalUnits _Instance;
	return &_Instance;
}

void CGlobalUnits::SetEmojiPath(const SStringT& emojiPath) {
	m_sstrEmojiFolder = emojiPath;
}

std::string CGlobalUnits::GenerateUUID()
{
	char szbuf[100];
	GUID guid;
	::CoCreateGuid(&guid);
	sprintf(szbuf,
		"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		guid.Data1,
		guid.Data2,
		guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);

	std::string strUUID = szbuf;
	return strUUID;
}

void CGlobalUnits::OperateEmojis()
{
	/*	2024-12-11	yangjinpeng
	*	������Ŀ¼�µ�emojiͼƬ
	*/
	SStringT sstrPath = m_sstrEmojiFolder + _T("*.png");

	WIN32_FIND_DATA findFileData;
	HANDLE hFind;
	hFind = FindFirstFile(sstrPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("No files found.\n");
		return;
	}
	else {
		do {
			SStringT sstrName = findFileData.cFileName;
			SStringA tmp = S_CT2A(sstrName);
			std::string strName(tmp.c_str(), tmp.GetLength());
			std::string strUUID = GenerateUUID();
			m_mapEmojisIndex.insert(std::make_pair(strUUID, strName));

			SStringT strPath = m_sstrEmojiFolder + sstrName;
			IBitmap* pRes = SResLoadFromFile::LoadImage(strPath);
			if (pRes)
				m_mapFace.insert(std::make_pair(strUUID, pRes));
		} while (FindNextFile(hFind, &findFileData) != 0);
		FindClose(hFind);
	}
}

void CGlobalUnits::GenerateShamDate()
{
	const char* shamAreas[] = {
		"����",
		"����",
		"�㶫",
		"����׳��������",
		"����",
		"�ӱ�",
		"ɽ��",
		"ɽ��",
		"������",
		"������",
		"�����",
		"�Ϻ���",
		"����",
		"����",
		"������",
		"�㽭",
		"�½�ά���������",
		"���Ļ���������",
		"����",
		"�ຣ",
		"����",
		"����",
		"����",
		"����",
		"����������",
		"�Ĵ�",
		"���ɹ�������",
		"̨��",
		"����",
		"����",
		"����",
		"����",
		"����ر�������",
		"�����ر�������",
	};

	const char* shamNames[] = {
		"����",
		"����",
		"����",
		"����",
		"��Ӣս����",
		"����׹Ǿ�",
		"���μ�",
		"ˮ䰴�",
		"��������",
		"��¥��",
		"��ƿ÷",
		"���㶨��"
	};

	const char* shamGroupNames[] = {
		"Ⱥ�Ĳ���1",
		"Ⱥ�Ĳ���2",
		"Ⱥ�Ĳ���3",
		"Ⱥ�Ĳ���4",
		"Ⱥ�Ĳ���5",
		"Ⱥ�Ĳ���6",
		"Ⱥ�Ĳ���7",
		"Ⱥ�Ĳ���8",
		"Ⱥ�Ĳ���9",
		"Ⱥ�Ĳ���10",
		"Ⱥ�Ĳ���11",
		"Ⱥ�Ĳ���12"
	};

	//�����ϵ�˵ļ�����
	{
		for (int i = 0; i < 50; i++)
		{
			std::string strUUID = GenerateUUID();
			int nNameIndex = rand() % 11;
			std::string strTempName = shamNames[nNameIndex];

			std::ostringstream os;
			os.str("");
			os << strTempName << i;
			std::string strName = os.str();
			int nAreaIndex = rand() % 33;
			std::string strArea = shamAreas[nAreaIndex];

			m_mapPersonals.insert(std::make_pair(strUUID,
				PERSONAL_INFO(strUUID, strName, "", strArea, "��֪��", "�������ڷ�Զ����������Ʒ�Ըߡ�")));
		}
	}

	//���Ⱥ�ļ�����
	{
		for (int i = 0; i < 12; i++)
		{
			std::string strUUID = GenerateUUID();
			//int nNameIndex = rand() % 11;
			std::string strTempName = shamGroupNames[i];

			std::ostringstream os;
			os.str("");
			os << strTempName << i;
			std::string strName = os.str();

			m_mapGroups.insert(std::make_pair(strUUID, GROUP_INFO(strUUID, strName, "��һ�����")));
		}
	}
}
