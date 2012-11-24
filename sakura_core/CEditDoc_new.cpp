/*!	@file
	@brief �����֘A���̊Ǘ�

	@author Norio Nakatani
	
	@date aroka �x���΍�ŕϐ�����
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta
	Copyright (C) 2001, Stonee, Misaka, hor, YAZAKI
	Copyright (C) 2002, hor, genta, aroka, Moca, MIK, ai
	Copyright (C) 2003, MIK, zenryaku, genta, little YOSHI
	Copyright (C) 2004, genta
	Copyright (C) 2005, genta, D.S.Koba, ryoji
	Copyright (C) 2006, ryoji
	Copyright (C) 2007, maru, Moca, genta
	Copyright (C) 2008, ryoji
	Copyright (C) 2009, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "StdAfx.h"
#include "CEditDoc.h"
#include "charcode.h"
#include "CFuncInfoArr.h"// 2002/2/10 aroka
#include "CDocLine.h"// 2002/2/10 aroka
#include "CEditWnd.h"
#include "Debug.h"
#include "etc_uty.h"
#include "my_icmp.h" // Nov. 29, 2002 genta/moca
#include "mymessage.h"	//	Oct. 9, 2004 genta
#include "CEditApp.h"	//	Oct. 9, 2004 genta

/* Java��̓��[�h */
enum EFuncListJavaMode {
	FL_JAVA_MODE_NORMAL = 0,
	FL_JAVA_MODE_WORD = 1,
	FL_JAVA_MODE_SYMBOL = 2,
	FL_JAVA_MODE_COMMENT = 8,
	FL_JAVA_MODE_SINGLE_QUOTE = 20,
	FL_JAVA_MODE_DOUBLE_QUOTE = 21,
	FL_JAVA_MODE_TOO_LONG_WORD = 999
};

/* Java�֐����X�g�쐬 */
void CEditDoc::MakeFuncList_Java( CFuncInfoArr* pcFuncInfoArr )
{
	const char*	pLine;
	int			nLineLen;
	int			nLineCount;
	int			i;
	int			nNestLevel;
	int			nCharChars;
	char		szWordPrev[100];
	char		szWord[100];
	int			nWordIdx = 0;
	int			nMaxWordLeng = 70;
	int			nMode;
	char		szFuncName[100];
	int			nFuncLine = 0;
	int			nFuncId;
	int			nFuncNum;
	char		szClass[1024];

	int			nClassNestArr[16];
	int			nClassNestArrNum;
	int			nNestLevel2Arr[16];

	nNestLevel = 0;
	szWordPrev[0] = '\0';
	szWord[nWordIdx] = '\0';
	nMode = FL_JAVA_MODE_NORMAL;
	nNestLevel2Arr[0] = 0;
	nFuncNum = 0;
	szClass[0] = '\0';
	nClassNestArrNum = 0;
	for( nLineCount = 0; nLineCount <  m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		pLine = m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		for( i = 0; i < nLineLen; ++i ){
			/* 1�o�C�g������������������ */
			//nCharChars = CMemory::MemCharNext( pLine, nLineLen, &pLine[i] ) - &pLine[i];
			nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, i );
			if(	1 < nCharChars ){
				i += (nCharChars - 1);
				continue;
			}

			/* �G�X�P�[�v�V�[�P���X�͏�Ɏ�菜�� */
			if( '\\' == pLine[i] ){
				++i;
			}else
			/* �V���O���N�H�[�e�[�V����������ǂݍ��ݒ� */
			if( FL_JAVA_MODE_SINGLE_QUOTE == nMode ){
				if( '\'' == pLine[i] ){
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else{
				}
			}else
			/* �_�u���N�H�[�e�[�V����������ǂݍ��ݒ� */
			if( FL_JAVA_MODE_DOUBLE_QUOTE == nMode ){
				if( '"' == pLine[i] ){
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else{
				}
			}else
			/* �R�����g�ǂݍ��ݒ� */
			if( FL_JAVA_MODE_COMMENT == nMode ){
				if( i < nLineLen - 1 && '*' == pLine[i] &&  '/' == pLine[i + 1] ){
					++i;
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else{
				}
			}
			/* �P��ǂݍ��ݒ� */
			else if( FL_JAVA_MODE_WORD == nMode ){
				if( '_' == pLine[i] ||
					':' == pLine[i] ||
					'~' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )||
					'.' == pLine[i]
				){
					++nWordIdx;
					if( nWordIdx >= nMaxWordLeng ){
						nMode = FL_JAVA_MODE_TOO_LONG_WORD;
						continue;
					}else{
						szWord[nWordIdx] = pLine[i];
						szWord[nWordIdx + 1] = '\0';
					}
				}else{
					/* �N���X�錾������������ */
					//	Oct. 10, 2002 genta interface���Ώۂ�
					if( 0 == strcmp( "class", szWordPrev ) ||
						0 == strcmp( "interface", szWordPrev )
					 ){
						nClassNestArr[nClassNestArrNum] = nNestLevel;
						++nClassNestArrNum;
						if( 0 < nNestLevel	){
							strcat( szClass, "\\" );
						}
						strcat( szClass, szWord );

						nFuncId = 0;
						++nFuncNum;
						/*
						  �J�[�\���ʒu�ϊ�
						  �����ʒu(�s������̃o�C�g���A�܂�Ԃ������s�ʒu)
						  ��
						  ���C�A�E�g�ʒu(�s������̕\�����ʒu�A�܂�Ԃ�����s�ʒu)
						*/
						int		nPosX;
						int		nPosY;
						m_cLayoutMgr.LogicToLayout(
							0,
							nLineCount/*nFuncLine - 1*/,
							&nPosX,
							&nPosY
						);
						char szWork[256];
						wsprintf( szWork, "%s::%s", szClass, "��`�ʒu" );
						pcFuncInfoArr->AppendData( nPosY + 1/*nFuncLine*/, nPosY + 1, szWork, nFuncId );

					}

					nMode = FL_JAVA_MODE_NORMAL;
					i--;
					continue;
				}
			}else
			/* �L����ǂݍ��ݒ� */
			if( FL_JAVA_MODE_SYMBOL == nMode ){
				if( '_' == pLine[i] ||
					':' == pLine[i] ||
					'~' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )||
					'\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i] ||
					'{' == pLine[i] ||
					'}' == pLine[i] ||
					'(' == pLine[i] ||
					')' == pLine[i] ||
					';' == pLine[i]	||
					'\'' == pLine[i] ||
					'"' == pLine[i] ||
					'/' == pLine[i] ||
					'.' == pLine[i]
				){
					nMode = FL_JAVA_MODE_NORMAL;
					i--;
					continue;
				}else{
				}
			}else
			/* ���߂���P�ꖳ���� */
			if( FL_JAVA_MODE_TOO_LONG_WORD == nMode ){
				/* �󔒂�^�u�L�������΂� */
				if( '\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i]
				){
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}
			}else
			/* �m�[�}�����[�h */
			if( FL_JAVA_MODE_NORMAL == nMode ){
				/* �󔒂�^�u�L�������΂� */
				if( '\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i]
				){
					continue;
				}else
				if( i < nLineLen - 1 && '/' == pLine[i] &&  '/' == pLine[i + 1] ){
					break;
				}else
				if( i < nLineLen - 1 && '/' == pLine[i] &&  '*' == pLine[i + 1] ){
					++i;
					nMode = FL_JAVA_MODE_COMMENT;
					continue;
				}else
				if( '\'' == pLine[i] ){
					nMode = FL_JAVA_MODE_SINGLE_QUOTE;
					continue;
				}else
				if( '"' == pLine[i] ){
					nMode = FL_JAVA_MODE_DOUBLE_QUOTE;
					continue;
				}else
				if( '{' == pLine[i] ){
					if( 0 < nClassNestArrNum && 2 == nNestLevel2Arr[nClassNestArrNum - 1] ){
						//	Oct. 10, 2002 genta
						//	���\�b�h���ł���Ƀ��\�b�h���`���邱�Ƃ͂Ȃ��̂�
						//	�l�X�g���x������ǉ� class/interface�̒����̏ꍇ�̂ݔ��肷��
						if( nClassNestArr[nClassNestArrNum - 1] == nNestLevel - 1
						 && 0 != strcmp( "sizeof", szFuncName )
						 && 0 != strcmp( "if", szFuncName )
						 && 0 != strcmp( "for", szFuncName )
						 && 0 != strcmp( "do", szFuncName )
						 && 0 != strcmp( "while", szFuncName )
						 && 0 != strcmp( "catch", szFuncName )
						 && 0 != strcmp( "switch", szFuncName )
						 && 0 != strcmp( "return", szFuncName )
						){
							nFuncId = 2;
							++nFuncNum;
							/*
							  �J�[�\���ʒu�ϊ�
							  �����ʒu(�s������̃o�C�g���A�܂�Ԃ������s�ʒu)
							  ��
							  ���C�A�E�g�ʒu(�s������̕\�����ʒu�A�܂�Ԃ�����s�ʒu)
							*/
							int		nPosX;
							int		nPosY;
							m_cLayoutMgr.LogicToLayout(
								0,
								nFuncLine - 1,
								&nPosX,
								&nPosY
							);
							char szWork[256];
							wsprintf( szWork, "%s::%s", szClass, szFuncName );
							pcFuncInfoArr->AppendData( nFuncLine, nPosY + 1, szWork, nFuncId );
						}
					}
					if( 0 < nClassNestArrNum ){
						nNestLevel2Arr[nClassNestArrNum - 1] = 0;
					}
					++nNestLevel;
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else
				if( '}' == pLine[i] ){
					if( 0 < nClassNestArrNum ){
						nNestLevel2Arr[nClassNestArrNum - 1] = 0;
					}

					nNestLevel--;
					if( 0 < nClassNestArrNum &&
						nClassNestArr[nClassNestArrNum - 1] == nNestLevel
					){
						nClassNestArrNum--;
						int k;
						for( k = lstrlen( szClass ) - 1; k >= 0; k-- ){
							if( '\\' == szClass[k] ){
								break;
							}
						}
						if( 0 > k ){
							k = 0;
						}
						szClass[k] = '\0';
					}
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else
				if( '(' == pLine[i] ){
					if( 0 < nClassNestArrNum /*nNestLevel == 1*/ &&
						0 != strcmp( "new", szWordPrev )
					){
						strcpy( szFuncName, szWord );
						nFuncLine = nLineCount + 1;
						if( 0 < nClassNestArrNum ){
							nNestLevel2Arr[nClassNestArrNum - 1] = 1;
						}
					}
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else
				if( ')' == pLine[i] ){
					int			k;
					const char*	pLine2;
					int			nLineLen2;
					int			nLineCount2;
					nLineCount2 = nLineCount;
					pLine2 = pLine;
					nLineLen2 = nLineLen;
					k = i + 1;
					BOOL		bCommentLoop;
					bCommentLoop = FALSE;
				loop_is_func:;
					for( ; k < nLineLen2; ++k ){
						if( !bCommentLoop ){
							if( pLine2[k] != ' ' && pLine2[k] != TAB && pLine2[k] != CR && pLine2[k] != LF ){
								if( k + 1 < nLineLen2 && pLine2[k] == '/' && pLine2[k + 1] == '*' ){
									bCommentLoop = TRUE;
									++k;
								}else
								if( k + 1 < nLineLen2 && pLine2[k] == '/' && pLine2[k + 1] == '/' ){
									k = nLineLen2 + 1;
									break;
								}else{
									break;
								}
							}
						}else{
							if( k + 1 < nLineLen2 && pLine2[k] == '*' && pLine2[k + 1] == '/' ){
								bCommentLoop = FALSE;
								++k;
							}
						}
					}
					if( k >= nLineLen2 ){
						k = 0;
						++nLineCount2;
						pLine2 = m_cDocLineMgr.GetLineStr( nLineCount2, &nLineLen2 );
						if( NULL != pLine2 ){
							goto loop_is_func;
						}
						if( 0 < nClassNestArrNum ){
							nNestLevel2Arr[nClassNestArrNum - 1] = 0;
						}
					}else{
						//	Oct. 10, 2002 genta
						//	abstract �ɂ��Ή�
						if( pLine2[k] == '{' || pLine2[k] == ';' ||
							__iscsym( pLine2[k] ) ){
							if( 0 < nClassNestArrNum ){
								if( 1 == nNestLevel2Arr[nClassNestArrNum - 1] ){
									nNestLevel2Arr[nClassNestArrNum - 1] = 2;
								}
							}
						}else{
							if( 0 < nClassNestArrNum ){
								nNestLevel2Arr[nClassNestArrNum - 1] = 0;
							}
						}
					}
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else
				if( ';' == pLine[i] ){
					if( 0 < nClassNestArrNum && 2 == nNestLevel2Arr[nClassNestArrNum - 1] ){
						//	Oct. 10, 2002 genta
						// �֐��̒��ŕʂ̊֐��̐錾�����g�����Ƃ��āCJava�ł���́H
						if( nClassNestArr[nClassNestArrNum - 1] == nNestLevel - 1
						 && 0 != strcmp( "sizeof", szFuncName )
						 && 0 != strcmp( "if", szFuncName )
						 && 0 != strcmp( "for", szFuncName )
						 && 0 != strcmp( "do", szFuncName )
						 && 0 != strcmp( "while", szFuncName )
						 && 0 != strcmp( "catch", szFuncName )
						 && 0 != strcmp( "switch", szFuncName )
						 && 0 != strcmp( "return", szFuncName )
						){
							nFuncId = 1;
							++nFuncNum;
							/*
							  �J�[�\���ʒu�ϊ�
							  �����ʒu(�s������̃o�C�g���A�܂�Ԃ������s�ʒu)
							  ��
							  ���C�A�E�g�ʒu(�s������̕\�����ʒu�A�܂�Ԃ�����s�ʒu)
							*/
							int		nPosX;
							int		nPosY;
							m_cLayoutMgr.LogicToLayout(
								0,
								nFuncLine - 1,
								&nPosX,
								&nPosY
							);
							char szWork[256];
							wsprintf( szWork, "%s::%s", szClass, szFuncName );
							pcFuncInfoArr->AppendData( nFuncLine, nPosY + 1, szWork, nFuncId );
						}
					}
					if( 0 < nClassNestArrNum ){
						nNestLevel2Arr[nClassNestArrNum - 1] = 0;
					}
					nMode = FL_JAVA_MODE_NORMAL;
					continue;
				}else{
					if( '_' == pLine[i] ||
						':' == pLine[i] ||
						'~' == pLine[i] ||
						('a' <= pLine[i] &&	pLine[i] <= 'z' )||
						('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
						('0' <= pLine[i] &&	pLine[i] <= '9' )||
						'.' == pLine[i]
					){
						strcpy( szWordPrev, szWord );
						nWordIdx = 0;
						szWord[nWordIdx] = pLine[i];
						szWord[nWordIdx + 1] = '\0';
						nMode = FL_JAVA_MODE_WORD;
					}else{
						nMode = FL_JAVA_MODE_NORMAL;
					}
				}
			}
		}
	}
#ifdef _DEBUG
	pcFuncInfoArr->DUMP();
#endif
	return;
}

//	From Here Aug. 14, 2000 genta
//
//	�����������֎~����Ă��邩�ǂ���
//	�߂�l: true: �֎~ / false: ����
//
bool CEditDoc::IsModificationForbidden( int nCommand )
{
	//	�ҏW�\�̏ꍇ
	if( IsEditable() )
		return false; // ��ɏ�����������

	//	�㏑���֎~���[�h�̏ꍇ
	//	�b��Case��: ���ۂɂ͂����ƌ����̗ǂ����@���g���ׂ�
	switch( nCommand ){
	//	�t�@�C��������������R�}���h�͎g�p�֎~
	case F_CHAR:
	case F_IME_CHAR:
	case F_DELETE:
	case F_DELETE_BACK:
	case F_WordDeleteToEnd:
	case F_WordDeleteToStart:
	case F_WordDelete:
	case F_WordCut:
	case F_LineDeleteToStart:
	case F_LineDeleteToEnd:
	case F_LineCutToStart:
	case F_LineCutToEnd:
	case F_DELETE_LINE:
	case F_CUT_LINE:
	case F_DUPLICATELINE:
	case F_INDENT_TAB:
	case F_UNINDENT_TAB:
	case F_INDENT_SPACE:
	case F_UNINDENT_SPACE:
	case F_CUT:
	case F_PASTE:
	case F_INS_DATE:
	case F_INS_TIME:
	case F_CTRL_CODE_DIALOG:	//@@@ 2002.06.02 MIK
	case F_INSTEXT:
	case F_ADDTAIL:
	case F_PASTEBOX:
	case F_REPLACE_DIALOG:
	case F_REPLACE:
	case F_REPLACE_ALL:
	case F_CODECNV_EMAIL:
	case F_CODECNV_EUC2SJIS:
	case F_CODECNV_UNICODE2SJIS:
	case F_CODECNV_UNICODEBE2SJIS:
	case F_CODECNV_SJIS2JIS:
	case F_CODECNV_SJIS2EUC:
	case F_CODECNV_UTF82SJIS:
	case F_CODECNV_UTF72SJIS:
	case F_CODECNV_SJIS2UTF7:
	case F_CODECNV_SJIS2UTF8:
	case F_CODECNV_AUTO2SJIS:
	case F_TOLOWER:
	case F_TOUPPER:
	case F_TOHANKAKU:
	case F_TOHANKATA:				// 2002/08/29 ai
	case F_TOZENEI:					// 2001/07/30 Misaka
	case F_TOHANEI:
	case F_TOZENKAKUKATA:
	case F_TOZENKAKUHIRA:
	case F_HANKATATOZENKATA:
	case F_HANKATATOZENHIRA:
	case F_TABTOSPACE:
	case F_SPACETOTAB:  //#### Stonee, 2001/05/27
	case F_HOKAN:
	case F_CHGMOD_INS:
	case F_LTRIM:		// 2001.12.03 hor
	case F_RTRIM:		// 2001.12.03 hor
	case F_SORT_ASC:	// 2001.12.11 hor
	case F_SORT_DESC:	// 2001.12.11 hor
	case F_MERGE:		// 2001.12.11 hor
	case F_UNDO:		// 2007.10.12 genta
	case F_REDO:		// 2007.10.12 genta
		return true;
	}
	return false;	//	�f�t�H���g�ŏ�����������
}
//	To Here Aug. 14, 2000 genta

//	From Here Aug. 21, 2000 genta
//
//	�����ۑ����s�����ǂ����̃`�F�b�N
//
void CEditDoc::CheckAutoSave(void)
{
	if( m_cAutoSave.CheckAction() ){
		//	�㏑���ۑ�

		if( !IsModified() )	//	�ύX�����Ȃ牽�����Ȃ�
			return;				//	�����ł́C�u���ύX�ł��ۑ��v�͖�������

		//	2003.10.09 zenryaku �ۑ����s�G���[�̗}��
		if( !IsFilePathAvailable() )	//	�܂��t�@�C�������ݒ肳��Ă��Ȃ���Εۑ����Ȃ�
			return;

		bool en = m_cAutoSave.IsEnabled();
		m_cAutoSave.Enable(false);	//	2�d�Ăяo����h������
		SaveFile( GetFilePath() );	//	�ۑ��im_nCharCode, m_cSaveLineCode��ύX���Ȃ��j
		m_cAutoSave.Enable(en);
	}
}

//
//	�ݒ�ύX�������ۑ�����ɔ��f����
//
void CEditDoc::ReloadAutoSaveParam(void)
{
	m_cAutoSave.SetInterval( m_pShareData->m_Common.GetAutoBackupInterval() );
	m_cAutoSave.Enable( m_pShareData->m_Common.IsAutoBackupEnabled() );
}


//	�t�@�C���̕ۑ��@�\��EditView����ړ�
//
bool CEditDoc::SaveFile( const char* pszPath )
{
	// 2006.09.01 ryoji �ۑ��O�������s�}�N�������s����
	RunAutoMacro( m_pShareData->m_nMacroOnSave, pszPath );

	if( FileWrite( pszPath, m_cSaveLineCode ) ){
		SetModified(false,true);	//	Jan. 22, 2002 genta

		/* ���݈ʒu�Ŗ��ύX�ȏ�ԂɂȂ������Ƃ�ʒm */
		m_cOpeBuf.SetNoModified();
		return true;
	}
	return false;
}

//	To Here Aug. 21, 2000 genta

//	From Here Sep 8, 2000 genta
//
//!	Perl�p�A�E�g���C����͋@�\�i�ȈՔŁj
/*!
	�P���� /^\\s*sub\\s+(\\w+)/ �Ɉ�v������ $1�����o��������s���B
	�l�X�g�Ƃ��͖ʓ|�������̂ōl���Ȃ��B
	package{ }���g��Ȃ���΂���ŏ\���D�������͂܂��B

	@par nMode�̈Ӗ�
	@li 0: �͂���
	@li 2: sub����������
	@li 1: �P��ǂݏo����

	@date 2005.06.18 genta �p�b�P�[�W��؂��\�� ::��'���l������悤��
*/
void CEditDoc::MakeFuncList_Perl( CFuncInfoArr* pcFuncInfoArr )
{
	const char*	pLine;
	int			nLineLen;
	int			nLineCount;
	int			i;
	int			nCharChars;
	char		szWord[100];
	int			nWordIdx = 0;
	int			nMaxWordLeng = 70;
	int			nMode;

	for( nLineCount = 0; nLineCount <  m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		pLine = m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		nMode = 0;
		for( i = 0; i < nLineLen; ++i ){
			/* 1�o�C�g������������������ */
			// 2005-09-02 D.S.Koba GetSizeOfChar
			nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, i );
			if(	1 < nCharChars ){
				break;
			}

			/* �P��ǂݍ��ݒ� */
			if( 0 == nMode ){
				/* �󔒂�^�u�L�������΂� */
				if( '\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i]
				){
					continue;
				}
				if( 's' != pLine[i] )
					break;
				//	sub �̈ꕶ���ڂ�������Ȃ�
				if( nLineLen - i < 4 )
					break;
				if( strncmp( pLine + i, "sub", 3 ) )
					break;
				int c = pLine[ i + 3 ];
				if( c == ' ' || c == '\t' ){
					nMode = 2;	//	����
					i += 3;
				}
				else
					break;
			}
			else if( 2 == nMode ){
				if( '\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i]
				){
					continue;
				}
				if( '_' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )
				){
					//	�֐����̎n�܂�
					nWordIdx = 0;
					szWord[nWordIdx] = pLine[i];
					szWord[nWordIdx + 1] = '\0';
					nMode = 1;
					continue;
				}
				else
					break;

			}
			else if( 1 == nMode ){
				if( '_' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )||
					//	Jun. 18, 2005 genta �p�b�P�[�W�C���q���l��
					//	�R������2�A�����Ȃ��Ƃ����Ȃ��̂����C�����͎蔲��
					':' == pLine[i] || '\'' == pLine[i]
				){
					++nWordIdx;
					if( nWordIdx >= nMaxWordLeng ){
						break;
					}else{
						szWord[nWordIdx] = pLine[i];
						szWord[nWordIdx + 1] = '\0';
					}
				}else{
					//	�֐����擾
					/*
					  �J�[�\���ʒu�ϊ�
					  �����ʒu(�s������̃o�C�g���A�܂�Ԃ������s�ʒu)
					  ��
					  ���C�A�E�g�ʒu(�s������̕\�����ʒu�A�܂�Ԃ�����s�ʒu)
					*/
					int		nPosX;
					int		nPosY;
					m_cLayoutMgr.LogicToLayout(
						0,
						nLineCount/*nFuncLine - 1*/,
						&nPosX,
						&nPosY
					);
					//	Mar. 9, 2001
					pcFuncInfoArr->AppendData( nLineCount + 1, nPosY + 1, szWord, 0 );

					break;
				}
			}
		}
	}
#ifdef _DEBUG
	pcFuncInfoArr->DUMP();
#endif
	return;
}
//	To HERE Sep. 8, 2000 genta








//	From Here June 23, 2001 N.Nakatani
//!	Visual Basic�֐����X�g�쐬�i�ȈՔŁj
/*!
	Visual Basic�̃R�[�h����P���Ƀ��[�U�[��`�̊֐���X�e�[�g�����g�����o��������s���B

    Jul 10, 2003 little YOSHI  �ׂ�����͂���悤�ɕύX
                               ���ׂẴL�[���[�h�͎����I�ɐ��`�����̂ŁA�啶���������͊��S�Ɉ�v����B
                               �t�H�[���⃂�W���[�������ł͂Ȃ��A�N���X�ɂ��Ή��B
							   �������AConst�́u,�v�ŘA���錾�ɂ͖��Ή�
	Jul. 21, 2003 genta �L�[���[�h�̑啶���E�������𓯈ꎋ����悤�ɂ���
	Aug  7, 2003 little YOSHI  �_�u���N�H�[�e�[�V�����ň͂܂ꂽ�e�L�X�g�𖳎�����悤�ɂ���
	                           �֐����Ȃǂ�VB�̖��O�t���K�����255�����Ɋg��
*/
void CEditDoc::MakeFuncList_VisualBasic( CFuncInfoArr* pcFuncInfoArr )
{
	const int	nMaxWordLeng = 255;	// Aug 7, 2003 little YOSHI  VB�̖��O�t���K�����255�����Ɋg��
	const char*	pLine;
	int			nLineLen = 0;//: 2002/2/3 aroka �x���΍�F������
	int			nLineCount;
	int			i;
	int			nCharChars;
	char		szWordPrev[256];	// Aug 7, 2003 little YOSHI  VB�̖��O�t���K�����255�����Ɋg��
	char		szWord[256];		// Aug 7, 2003 little YOSHI  VB�̖��O�t���K�����255�����Ɋg��
	int			nWordIdx = 0;
//	int			nMaxWordLeng = 70;	// Aug 7, 2003 little YOSHI  �萔�ɕύX��
	int			nMode;
	char		szFuncName[256];	// Aug 7, 2003 little YOSHI  VB�̖��O�t���K�����255�����Ɋg��
	int			nFuncLine;
	int			nFuncId;
	int			nParseCnt = 0;
	bool		bClass;			// �N���X���W���[���t���O
	bool		bProcedure;		// �v���V�[�W���t���O�i�v���V�[�W�����ł�True�j
	bool		bDQuote;		// �_�u���N�H�[�e�[�V�����t���O�i�_�u���N�H�[�e�[�V������������True�j

	// ���ׂ�t�@�C�����N���X���W���[���̂Ƃ���Type�AConst�̋������قȂ�̂Ńt���O�𗧂Ă�
	bClass	= false;
	int filelen = _tcslen(GetFilePath());
	if ( 4 < filelen ) {
		if ( 0 == my_stricmp((GetFilePath() + filelen - 4), ".cls") ) {
			bClass	= true;
		}
	}

	szWordPrev[0] = '\0';
	szWord[nWordIdx] = '\0';
	nMode = 0;
	pLine = NULL;
	bProcedure	= false;
	for( nLineCount = 0; nLineCount <  m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		if( NULL != pLine ){
			if( '_' != pLine[nLineLen-1]){
				nParseCnt = 0;
			}
		}
		pLine = m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		nFuncId = 0;
		bDQuote	= false;
		for( i = 0; i < nLineLen; ++i ){
			/* 1�o�C�g������������������ */
			// 2005-09-02 D.S.Koba GetSizeOfChar
			nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, i );
			if(	0 == nCharChars ){
				nCharChars = 1;
			}
			/* �P��ǂݍ��ݒ� */
			if( 1 == nMode ){
				if( (1 == nCharChars && (
					'_' == pLine[i] ||
					'~' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )
					) )
				 || 2 == nCharChars
				){
					if( nWordIdx >= nMaxWordLeng ){
						nMode = 999;
						continue;
					}else{
						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);
					}
				} else if (1 == nCharChars && '"' == pLine[i]) {
					// Aug 7, 2003 little YOSHI  �ǉ�
					// �e�L�X�g�̒��͖������܂��B
					nMode	= 3;
				}else{
					if ( 0 == nParseCnt && 0 == my_stricmp(szWord, "Public") ) {
						// �p�u���b�N�錾���������I
						nFuncId |= 0x10;
					}else
					if ( 0 == nParseCnt && 0 == my_stricmp(szWord, "Private") ) {
						// �v���C�x�[�g�錾���������I
						nFuncId |= 0x20;
					}else
					if ( 0 == nParseCnt && 0 == my_stricmp(szWord, "Friend") ) {
						// �t�����h�錾���������I
						nFuncId |= 0x30;
					}else
					if ( 0 == nParseCnt && 0 == my_stricmp(szWord, "Static") ) {
						// �X�^�e�B�b�N�錾���������I
						nFuncId |= 0x100;
					}else
					if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Function" ) ){
						if ( 0 == my_stricmp( szWordPrev, "End" ) ){
							// �v���V�[�W���t���O���N���A
							bProcedure	= false;
						}else
						if( 0 != my_stricmp( szWordPrev, "Exit" ) ){
							if( 0 == my_stricmp( szWordPrev, "Declare" ) ){
								nFuncId |= 0x200;	// DLL�Q�Ɛ錾
							}else{
								bProcedure	= true;	// �v���V�[�W���t���O���Z�b�g
							}
							nFuncId |= 0x01;		// �֐�
							nParseCnt = 1;
							nFuncLine = nLineCount + 1;
						}
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Sub" ) ){
						if ( 0 == my_stricmp( szWordPrev, "End" ) ){
							// �v���V�[�W���t���O���N���A
							bProcedure	= false;
						}else
						if( 0 != my_stricmp( szWordPrev, "Exit" ) ){
							if( 0 == my_stricmp( szWordPrev, "Declare" ) ){
								nFuncId |= 0x200;	// DLL�Q�Ɛ錾
							}else{
								bProcedure	= true;	// �v���V�[�W���t���O���Z�b�g
							}
							nFuncId |= 0x02;		// �֐�
							nParseCnt = 1;
							nFuncLine = nLineCount + 1;
						}
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Get" )
					 && 0 == my_stricmp( szWordPrev, "Property" )
					){
						bProcedure	= true;	// �v���V�[�W���t���O���Z�b�g
						nFuncId	|= 0x03;		// �v���p�e�B�擾
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Let" )
					 && 0 == my_stricmp( szWordPrev, "Property" )
					){
						bProcedure	= true;	// �v���V�[�W���t���O���Z�b�g
						nFuncId |= 0x04;		// �v���p�e�B�ݒ�
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Set" )
					 && 0 == my_stricmp( szWordPrev, "Property" )
					){
						bProcedure	= true;	// �v���V�[�W���t���O���Z�b�g
						nFuncId |= 0x05;		// �v���p�e�B�Q��
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Const" )
					 && 0 != my_stricmp( szWordPrev, "#" )
					){
						if ( bClass || bProcedure || 0 == ((nFuncId >> 4) & 0x0f) ) {
							// �N���X���W���[���ł͋����I��Private
							// �v���V�[�W�����ł͋����I��Private
							// Public�̎w�肪�Ȃ��Ƃ��A�f�t�H���g��Private�ɂȂ�
							nFuncId &= 0x0f2f;
							nFuncId	|= 0x20;
						}
						nFuncId	|= 0x06;		// �萔
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Enum" )
					){
						nFuncId	|= 0x207;		// �񋓌^�錾
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Type" )
					){
						if ( bClass ) {
							// �N���X���W���[���ł͋����I��Private
							nFuncId &= 0x0f2f;
							nFuncId	|= 0x20;
						}
						nFuncId	|= 0x208;		// ���[�U��`�^�錾
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Event" )
					){
						nFuncId	|= 0x209;		// �C�x���g�錾
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}
					else if( 0 == nParseCnt && 0 == my_stricmp( szWord, "Property" )
					 && 0 == my_stricmp( szWordPrev, "End")
					){
						bProcedure	= false;	// �v���V�[�W���t���O���N���A
					}
					else if( 1 == nParseCnt ){
						strcpy( szFuncName, szWord );
						/*
						  �J�[�\���ʒu�ϊ�
						  �����ʒu(�s������̃o�C�g���A�܂�Ԃ������s�ʒu)
						  �� ���C�A�E�g�ʒu(�s������̕\�����ʒu�A�܂�Ԃ�����s�ʒu)
						*/
						int		nPosX;
						int		nPosY;
						m_cLayoutMgr.LogicToLayout(	0, nFuncLine - 1, &nPosX, &nPosY );
						pcFuncInfoArr->AppendData( nFuncLine, nPosY + 1 , szFuncName, nFuncId );
						nParseCnt = 0;
						nFuncId	= 0;	// Jul 10, 2003  little YOSHI  �_���a���g�p���邽�߁A�K��������
					}

					strcpy( szWordPrev, szWord );
					nWordIdx = 0;
					szWord[0] = '\0';
					nMode = 0;
					i--;
					continue;
				}
			}
			/* �L����ǂݍ��ݒ� */
			else if( 2 == nMode ){
				// Jul 10, 2003  little YOSHI
				// �u#Const�v�ƁuConst�v����ʂ��邽�߂ɁA�u#�v�����ʂ���悤�ɕύX
				if( '_' == pLine[i] ||
					'~' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )||
					'\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i] ||
					'{' == pLine[i] ||
					'}' == pLine[i] ||
					'(' == pLine[i] ||
					')' == pLine[i] ||
					';' == pLine[i]	||
					'\'' == pLine[i] ||
					'/' == pLine[i]	||
					'-' == pLine[i] ||
					'#' == pLine[i]
				){
					strcpy( szWordPrev, szWord );
					nWordIdx = 0;
					szWord[0] = '\0';
					nMode = 0;
					i--;
					continue;
				} else if (1 == nCharChars && '"' == pLine[i]) {
					// Aug 7, 2003 little YOSHI  �ǉ�
					// �e�L�X�g�̒��͖������܂��B
					nMode	= 3;
				}else{
					if( nWordIdx >= nMaxWordLeng ){
						nMode = 999;
						continue;
					}else{
						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);
					}
				}
			}else
			/* ���߂���P�ꖳ���� */
			if( 999 == nMode ){
				/* �󔒂�^�u�L�������΂� */
				if( '\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i]
				){
					nMode = 0;
					continue;
				}
			}else
			/* �m�[�}�����[�h */
			if( 0 == nMode ){
				/* �󔒂�^�u�L�������΂� */
				if( '\t' == pLine[i] ||
					' ' == pLine[i] ||
					CR == pLine[i] ||
					LF == pLine[i]
				){
					continue;
				}else
				if( i < nLineLen && '\'' == pLine[i] ){
					break;
				} else if (1 == nCharChars && '"' == pLine[i]) {
					// Aug 7, 2003 little YOSHI  �ǉ�
					// �e�L�X�g�̒��͖������܂��B
					nMode	= 3;
				}else{
					if( (1 == nCharChars && (
						'_' == pLine[i] ||
						'~' == pLine[i] ||
						('a' <= pLine[i] &&	pLine[i] <= 'z' )||
						('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
						('0' <= pLine[i] &&	pLine[i] <= '9' )
						) )
					 || 2 == nCharChars
					){
						nWordIdx = 0;

						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);

						nMode = 1;
					}else{
						nWordIdx = 0;
						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);

						nMode = 2;
					}
				}
			} else
			/* �e�L�X�g������܂œǂݔ�΂� */	// Aug 7, 2003 little YOSHI  �ǉ�
			if (nMode == 3) {
				// �A������_�u���N�H�[�e�[�V�����͖�������
				if (1 == nCharChars && '"' == pLine[i]) {
					// �_�u���N�H�[�e�[�V���������ꂽ��t���O�𔽓]����
					bDQuote	= !bDQuote;
				} else if (bDQuote) {
					// �_�u���N�H�[�e�[�V�����̎���
					// �_�u���N�H�[�e�[�V�����ȊO�̕��������ꂽ��m�[�}�����[�h�Ɉڍs
					--i;
					nMode	= 0;
					bDQuote	= false;
					continue;
				}
			}
			i += (nCharChars - 1);
		}
	}
	return;
}
//	To Here June 23, 2001 N.Nakatani


// From Here 2001.12.03 hor
/*! �u�b�N�}�[�N���X�g�쐬�i������I�j

	@date 2002.01.19 aroka ��s���}�[�N�Ώۂɂ���t���O bMarkUpBlankLineEnable �𓱓����܂����B
	@date 2005.10.11 ryoji "��@" �̉E�Q�o�C�g���S�p�󔒂Ɣ��肳�����̑Ώ�
	@date 2005.11.03 genta �����񒷏C���D�E�[�̃S�~������
*/
void CEditDoc::MakeFuncList_BookMark( CFuncInfoArr* pcFuncInfoArr )
{
	const char*	pLine;
	int		nLineLen;
	int		nLineCount;
	int		leftspace, pos_wo_space, k,nX,nY;
	char*	pszText;
	BOOL	bMarkUpBlankLineEnable = m_pShareData->m_Common.m_bMarkUpBlankLineEnable;	//! ��s���}�[�N�Ώۂɂ���t���O 20020119 aroka
	int		nNewLineLen	= m_cNewLineCode.GetLen();
	int		nLineLast	= m_cDocLineMgr.GetLineCount();
	int		nCharChars;

	for( nLineCount = 0; nLineCount <  nLineLast; ++nLineCount ){
		if(!m_cDocLineMgr.GetLine(nLineCount)->IsBookMarked())continue;
		pLine = m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		if( NULL == pLine ){
			break;
		}
		// Jan, 16, 2002 hor
		if( bMarkUpBlankLineEnable ){// 20020119 aroka
			if( nLineLen<=nNewLineLen && nLineCount< nLineLast ){
			  continue;
			}
		}// LTrim
		for( leftspace = 0; leftspace < nLineLen; ++leftspace ){
			if( pLine[leftspace] == ' ' ||
				pLine[leftspace] == '\t'){
				continue;
			}else if( (unsigned char)pLine[leftspace] == (unsigned char)0x81
				&& (unsigned char)pLine[leftspace + 1] == (unsigned char)0x40 ){
				++leftspace;
				continue;
			}
			break;
		}
		
		if( bMarkUpBlankLineEnable ){// 20020119 aroka
			if(( leftspace >= nLineLen-nNewLineLen && nLineCount< nLineLast )||
				( leftspace >= nLineLen )) {
				continue;
			}
		}// RTrim
		// 2005.10.11 ryoji �E����k��̂ł͂Ȃ�������T���悤�ɏC���i"��@" �̉E�Q�o�C�g���S�p�󔒂Ɣ��肳�����̑Ώ��j
		k = pos_wo_space = leftspace;
		while( k < nLineLen ){
			nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, k );
			if( 1 == nCharChars ){
				if( !(pLine[k] == CR ||
						pLine[k] == LF ||
						pLine[k] == SPACE ||
						pLine[k] == TAB ||
						pLine[k] == '\0') )
					pos_wo_space = k + nCharChars;
			}
			else if( 2 == nCharChars ){
				if( !((unsigned char)pLine[k] == (unsigned char)0x81 && (unsigned char)pLine[k + 1] == (unsigned char)0x40) )
					pos_wo_space = k + nCharChars;
			}
			k += nCharChars;
		}
		//	Nov. 3, 2005 genta �����񒷌v�Z���̏C��
		nLineLen = pos_wo_space - leftspace;
		pszText = new char[nLineLen + 1];
		memcpy( pszText, (const char *)&pLine[leftspace], nLineLen );
		pszText[nLineLen] = '\0';
		m_cLayoutMgr.LogicToLayout(	0, nLineCount, &nX, &nY );
		pcFuncInfoArr->AppendData( nLineCount+1, nY+1 , (char *)pszText, 0 );
		delete [] pszText;
	}
	return;
}
// To Here 2001.12.03 hor

// From Here Jan. 22, 2002 genta
/*! �ύX�t���O�̐ݒ�

	@param flag [in] �ݒ肷��l�Dtrue: �ύX�L�� / false: �ύX����
	@param redraw [in] true: �^�C�g���̍ĕ`����s�� / false: �s��Ȃ�
	
	@author genta
	@date 2002.01.22 �V�K�쐬
*/
void CEditDoc::SetModified( bool flag, bool redraw)
{
	if( m_bIsModified == flag )	//	�ύX���Ȃ���Ή������Ȃ�
		return;

	m_bIsModified = flag;
	if( redraw )
		UpdateCaption();
}
// From Here Jan. 22, 2002 genta

/*!
	�t�@�C�����̐ݒ�
	
	�t�@�C������ݒ肷��Ɠ����ɁC�E�B���h�E�A�C�R����K�؂ɐݒ肷��D
	
	@param szFile [in] �t�@�C���̃p�X��
	
	@author genta
	@date 2002.09.09
*/
void CEditDoc::SetFilePathAndIcon(const TCHAR* szFile)
{
	strcpy( m_szFilePath, szFile );
	SetDocumentIcon();
}

/*! �t�@�C����(�p�X�Ȃ�)���擾����
	@author Moca
	@date 2002.10.13
*/
const char * CEditDoc::GetFileName( void ) const
{
	const char *p, *pszName;
	pszName = p = GetFilePath();
	while( *p != '\0'  ){
		if( _IS_SJIS_1( (unsigned char)*p ) && _IS_SJIS_2( (unsigned char)p[1] ) ){
			p+=2;
		}else if( *p == '\\' ){
			pszName = p + 1;
			p++;
		}else{
			p++;
		}
	}
	return pszName;
}

/*!
	�A�C�R���̐ݒ�
	
	�^�C�v�ʐݒ�ɉ����ăE�B���h�E�A�C�R�����t�@�C���Ɋ֘A�Â���ꂽ���C
	�܂��͕W���̂��̂ɐݒ肷��D
	
	@author genta
	@date 2002.09.10
*/
void CEditDoc::SetDocumentIcon(void)
{
	HICON	hIconBig, hIconSmall;
	
	if( m_bGrepMode )	// Grep���[�h�̎��̓A�C�R����ύX���Ȃ�
		return;
	
	if( GetDocumentAttribute().m_bUseDocumentIcon )
		m_pcEditWnd->GetRelatedIcon( GetFilePath(), &hIconBig, &hIconSmall );
	else
		m_pcEditWnd->GetDefaultIcon( &hIconBig, &hIconSmall );

	m_pcEditWnd->SetWindowIcon( hIconBig, ICON_BIG );
	m_pcEditWnd->SetWindowIcon( hIconSmall, ICON_SMALL );
}

/*!
	�J�����g�t�@�C����MRU�ɓo�^����B
	�u�b�N�}�[�N���ꏏ�ɓo�^����B

	@date 2003.03.30 genta �쐬

*/
void CEditDoc::AddToMRU(void)
{
	EditInfo	fi;
	CMRU		cMRU;

	SetFileInfo( &fi );
	strcpy( fi.m_szMarkLines, m_cDocLineMgr.GetBookMarks() );

	//@@@ 2001.12.26 YAZAKI MRU���X�g�́ACMRU�Ɉ˗�����
	cMRU.Add( &fi );
}


/*!	@brief �w�肳�ꂽ�t�@�C�����J��

	���݂̕ҏW�󋵂ɉ����āC���݂̃E�B���h�E�ɓǂݍ��ނ��C�V�����E�B���h�E���J����
	���邢�͊��ɊJ����Ă���E�B���h�E�Ƀt�H�[�J�X���ڂ������ɂ��邩�����肵�C
	���s����D

	�Ώۃt�@�C���C�G�f�B�^�E�B���h�E�̏󋵂Ɉ˂炸�V�����t�@�C�����J�������ꍇ��
	�g�p����D

	@date 2003.03.30 genta �u���ĊJ���v���痘�p���邽�߂Ɉ����ǉ�
	@date 2004.10.09 CEditView���ړ�
	@date 2007.03.12 maru �d���R�[�h(���d�I�[�v�����������Ȃ�)��CShareData::IsPathOpened�Ɉړ�
*/
void CEditDoc::OpenFile( const char *filename, ECodeType nCharCode, BOOL bReadOnly )
{
	char		pszPath[_MAX_PATH];
	BOOL		bOpened;
	HWND		hWndOwner;

	/* �u�t�@�C�����J���v�_�C�A���O */
	if( filename == NULL ){
		pszPath[0] = '\0';
		if( !OpenFileDialog( m_hWnd, NULL, pszPath, &nCharCode, &bReadOnly ) ){
			return;
		}
	}
	else {
		//	2007.10.01 genta ���΃p�X���΃p�X�ɕϊ�
		//	�ϊ����Ȃ���IsPathOpened�Ő��������ʂ�����ꂸ�C
		//	����t�@�C���𕡐��J�����Ƃ�����D
		if( ! GetLongFileName( filename, pszPath )){
			//	�t�@�C�����̕ϊ��Ɏ��s
			::MYMESSAGEBOX( m_hWnd, MB_OK , GSTR_APPNAME, 
				_T("�t�@�C�����̕ϊ��Ɏ��s���܂��� [%s]"), filename );
			return;
		}
	}
	/* �w��t�@�C�����J����Ă��邩���ׂ� */
	if( CShareData::getInstance()->IsPathOpened(pszPath, &hWndOwner, nCharCode) ){		// �J���Ă���΃A�N�e�B�u�ɂ���
		/* 2007.03.12 maru �J���Ă����Ƃ��̏�����CShareData::IsPathOpened�Ɉړ� */
	}else{
		/* �t�@�C�����J����Ă��Ȃ� */
		/* �ύX�t���O���I�t�ŁA�t�@�C����ǂݍ���ł��Ȃ��ꍇ */
//@@@ 2001.12.26 YAZAKI Grep���ʂŖ����ꍇ���܂߂�B
		if( IsFileOpenInThisWindow()
		){
			BOOL bRet;
			/* �t�@�C���ǂݍ��� */
			//	Oct. 03, 2004 genta �R�[�h�m�F�͐ݒ�Ɉˑ�
			bRet = FileRead( pszPath, &bOpened, nCharCode, bReadOnly,
							m_pShareData->m_Common.m_bQueryIfCodeChange );

			// 2006.09.01 ryoji �I�[�v���㎩�����s�}�N�������s����
			// 2007.06.27 maru ���łɕҏW�E�B���h�E�͊J���Ă���̂ŁAFileRead���L�����Z�����ꂽ�ꍇ�͊J���}�N���͎��s�s�v
			if(TRUE==bRet) RunAutoMacro( m_pShareData->m_nMacroOnOpened );
		}else{
			if( strchr( pszPath, ' ' ) ){
				char	szFile2[_MAX_PATH + 3];
				wsprintf( szFile2, "\"%s\"", pszPath );
				strcpy( pszPath, szFile2 );
			}
			/* �V���ȕҏW�E�B���h�E���N�� */
			CEditApp::OpenNewEditor( m_hInstance, m_hWnd, pszPath, nCharCode, bReadOnly );
		}
	}
	return;
}

/*!	���C�A�E�g�p�����[�^�̕ύX

	��̓I�ɂ̓^�u���Ɛ܂�Ԃ��ʒu��ύX����D
	���݂̃h�L�������g�̃��C�A�E�g�݂̂�ύX���C���ʐݒ�͕ύX���Ȃ��D

	@date 2005.08.14 genta �V�K�쐬
	@date 2008.06.18 ryoji ���C�A�E�g�ύX�r���̓J�[�\���ړ��̉�ʃX�N���[���������Ȃ��i��ʂ̂�����}�~�j
*/
void CEditDoc::ChangeLayoutParam( bool bShowProgress, int nTabSize, int nMaxLineSize )
{
	HWND		hwndProgress = NULL;
	if( bShowProgress && NULL != m_pcEditWnd ){
		hwndProgress = m_pcEditWnd->m_hwndProgressBar;
		//	Status Bar���\������Ă��Ȃ��Ƃ���m_hwndProgressBar == NULL
	}

	if( hwndProgress ){
		::ShowWindow( hwndProgress, SW_SHOW );
	}

	//	���W�̕ۑ�
	int* posSave = SavePhysPosOfAllView();

	//	���C�A�E�g�̍X�V
	m_cLayoutMgr.ChangeLayoutParam( NULL, nTabSize, nMaxLineSize );

	//	���W�̕���
	//	���C�A�E�g�ύX�r���̓J�[�\���ړ��̉�ʃX�N���[���������Ȃ�	// 2008.06.18 ryoji
	SetDrawSwitchOfAllViews( FALSE );
	RestorePhysPosOfAllView( posSave );
	SetDrawSwitchOfAllViews( TRUE );

	for( int i = 0; i < GetAllViewCount(); i++ ){
		if( m_cEditViewArr[i].m_hWnd ){
			InvalidateRect( m_cEditViewArr[i].m_hWnd, NULL, TRUE );
			m_cEditViewArr[i].AdjustScrollBars();	// 2008.06.18 ryoji
		}
	}
	if( !GetDocumentAttribute().m_bLineNumIsCRLF ){
		ActiveView().DrawCaretPosInfo();	// 2009.07.25 ryoji
	}

	if( hwndProgress ){
		::ShowWindow( hwndProgress, SW_HIDE );
	}
}

/* ����(����)

	@date 2006.12.30 ryoji CEditView::Command_FILESAVEAS()���珈���{�̂�؂�o��
*/
void CEditDoc::FileClose( void )
{
	/* �t�@�C�������Ƃ���MRU�o�^ & �ۑ��m�F & �ۑ����s */
	if( !OnFileClose() ){
		return;
	}
	/* �����f�[�^�̃N���A */
	InitDoc();

	/* �S�r���[�̏����� */
	InitAllView();

	/* �e�E�B���h�E�̃^�C�g�����X�V */
	UpdateCaption();

	// 2006.09.01 ryoji �I�[�v���㎩�����s�}�N�������s����
	RunAutoMacro( m_pShareData->m_nMacroOnOpened );

	return;
}

/* ���ĊJ��

	@param filename	[in] �t�@�C����
	@param nCharCode	[in] �����R�[�h
	@param bReadOnly	[in] �ǂݎ���p��

	@date 2006.12.30 ryoji CEditView::Command_FILESAVEAS()���珈���{�̂�؂�o��
*/
void CEditDoc::FileCloseOpen( const char *filename, ECodeType nCharCode, BOOL bReadOnly )
{
	/* �t�@�C�������Ƃ���MRU�o�^ & �ۑ��m�F & �ۑ����s */
	if( !OnFileClose() ){
		return;
	}

	// Mar. 30, 2003 genta
	char	pszPath[_MAX_PATH];

	if( filename == NULL ){
		pszPath[0] = '\0';
		if( !OpenFileDialog( m_hWnd, NULL, pszPath, &nCharCode, &bReadOnly ) ){
			return;
		}
	}

	/* �����f�[�^�̃N���A */
	InitDoc();

	/* �S�r���[�̏����� */
	InitAllView();

	/* �e�E�B���h�E�̃^�C�g�����X�V */
	UpdateCaption();

	/* �t�@�C�����J�� */
	// Mar. 30, 2003 genta
	// Oct.  9, 2004 genta CEditDoc�ֈړ��������Ƃɂ��ύX
	OpenFile(( filename ? filename : pszPath ), nCharCode, bReadOnly );
}

/*! �㏑���ۑ�

	@param warnbeep [in] true: �ۑ��s�v or �ۑ��֎~�̂Ƃ��Ɍx�����o��
	@param askname	[in] true: �t�@�C�������ݒ�̎��ɓ��͂𑣂�

	@date 2006.12.30 ryoji CEditView::Command_FILESAVE()���珈���{�̂�؂�o��
*/
BOOL CEditDoc::FileSave( bool warnbeep, bool askname )
{

	/* ���ύX�ł��㏑�����邩 */
	if( FALSE == m_pShareData->m_Common.m_bEnableUnmodifiedOverwrite
	 && !IsModified()	// �ύX�t���O
	 ){
	 	//	Feb. 28, 2004 genta
	 	//	�ۑ��s�v�ł��x�������o���ė~�����Ȃ��ꍇ������
	 	if( warnbeep ){
			ErrorBeep();
		}
		return TRUE;
	}

	if( !IsFilePathAvailable() ){
		if( ! askname ){
			return FALSE;
		}
		//	Feb. 28, 2004 genta SAVEAS�̌��ʂ��������Ԃ���Ă��Ȃ�����
		//	���̏����Ƒg�ݍ��킹��Ƃ��ɖ�肪������
		//return Command_FILESAVEAS_DIALOG();
		FileSaveAs_Dialog();
	}
	else {
		//	Jun.  5, 2004 genta
		//	�ǂݎ���p�̃`�F�b�N��CEditDoc����㏑���ۑ������Ɉړ�
		if( m_bReadOnly ){	/* �ǂݎ���p���[�h */
			if( warnbeep ){
				ErrorBeep();
				MYMESSAGEBOX(
					m_hWnd,
					MB_OK | MB_ICONSTOP | MB_TOPMOST,
					GSTR_APPNAME,
					_T("%s\n\n�͓ǂݎ���p���[�h�ŊJ���Ă��܂��B �㏑���ۑ��͂ł��܂���B\n\n")
					_T("���O��t���ĕۑ�������΂����Ǝv���܂��B"),
					IsFilePathAvailable() ? GetFilePath() : _T("(����)")
				);
			}
			return FALSE;
		}

		if( SaveFile( GetFilePath() ) ){	//	m_nCharCode, m_cSaveLineCode��ύX�����ɕۑ�
			/* �L�����b�g�̍s���ʒu��\������ */
			m_cEditViewArr[m_nActivePaneIndex].DrawCaretPosInfo();
			return TRUE;
		}
	}
	return FALSE;
}

/*! ���O��t���ĕۑ��_�C�A���O

	@date 2006.12.30 ryoji CEditView::Command_FILESAVEAS_DIALOG()���珈���{�̂�؂�o��
*/
BOOL CEditDoc::FileSaveAs_Dialog( void )
{
	//	Aug. 16, 2000 genta
	//	���݂̃t�@�C�����������l�ŗ^���Ȃ�
	//	May 18, 2001 genta
	//	���݂̃t�@�C������^���Ȃ��̂͏㏑���֎~�̎��̂�
	//	�����łȂ��ꍇ�ɂ͌��݂̃t�@�C�����������l�Ƃ��Đݒ肷��B
	char szPath[_MAX_PATH + 1];
	if( IsReadOnly() )
		szPath[0] = '\0';
	else
		strcpy( szPath, GetFilePath() );

	//	Feb. 9, 2001 genta
	//	Jul. 26, 2003 ryoji BOM�̗L����^����p�����[�^
	if( SaveFileDialog( szPath, &m_nCharCode, &m_cSaveLineCode, &m_bBomExist ) ){
		//	Jun.  5, 2004 genta
		//	�ǂݎ���p�̃`�F�b�N��CEditDoc����㏑���ۑ������Ɉړ�
		//	�����ŏ㏑�������̂�h��
		if( m_bReadOnly && strcmp( szPath, GetFilePath()) == 0 ){
			ErrorBeep();
			MYMESSAGEBOX(
				m_hWnd,
				MB_OK | MB_ICONSTOP | MB_TOPMOST,
				GSTR_APPNAME,
				_T("�ǂݎ���p���[�h�ł͓���t�@�C���ւ̏㏑���ۑ��͂ł��܂���B")
			);
		}
		else {
			//Command_FILESAVEAS( szPath );
			FileSaveAs( szPath );
			m_cSaveLineCode = EOL_NONE;	// 2008.03.20 ryoji ���s�R�[�h�͈����p���Ȃ��i�㏑���ۑ��ł͏�Ɂu�ϊ��Ȃ��v�j
			return TRUE;
		}
	}
	m_cSaveLineCode = EOL_NONE;	// 2008.03.20 ryoji ���s�R�[�h�͈����p���Ȃ��i�㏑���ۑ��ł͏�Ɂu�ϊ��Ȃ��v�j
	return FALSE;
}

/* ���O��t���ĕۑ�

	@param filename	[in] �t�@�C����

	@date 2006.12.30 ryoji CEditView::Command_FILESAVEAS()���珈���{�̂�؂�o��
*/
BOOL CEditDoc::FileSaveAs( const char *filename )
{
	if( SaveFile( filename ) ){
		/* �L�����b�g�̍s���ʒu��\������ */
		m_cEditViewArr[m_nActivePaneIndex].DrawCaretPosInfo();
		OnChangeSetting();	//	�^�C�v�ʐݒ�̕ύX���w���B
		//	�ăI�[�v��
		//	Jul. 26, 2003 ryoji ���݊J���Ă���̂Ɠ����R�[�h�ŊJ������
		ReloadCurrentFile( m_nCharCode, FALSE );
		return TRUE;
	}
	return FALSE;
}


/*!
	CEditView�̉�ʃo�b�t�@���폜
	@date 2007.09.09 Moca �V�K�쐬
*/
void CEditDoc::DeleteCompatibleBitmap()
{
	// CEditView�Q�֓]������
	for( int i = 0; i < GetAllViewCount(); i++ ){
		if( m_cEditViewArr[i].m_hWnd ){
			m_cEditViewArr[i].DeleteCompatibleBitmap();
		}
	}
}
/*[EOF]*/