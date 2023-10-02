//
// Created by KimByoungGook on 2021-12-14.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "com/dap_com.h"


int	func_conf_lib_Init(char *Pre_Configure_Path_File_Name, struct _FUNC_CONF_LIB_config_struct **Pre_Comm_Lib_Conf_Struct)
{
    int		local_item_count = 0;

    struct _FUNC_CONF_LIB_config_struct	*local_Comm_Lib_Conf_Struct = *Pre_Comm_Lib_Conf_Struct;

    if ((local_item_count = func_conf_lib_Test_Parser(Pre_Configure_Path_File_Name)) <= 0)
    {
        Pre_Comm_Lib_Conf_Struct = NULL;

        return -1;
    }

    size_t nStSize = sizeof(func_conf_lib_config_struct) * (size_t)local_item_count;
    local_Comm_Lib_Conf_Struct = (struct _FUNC_CONF_LIB_config_struct *) malloc(nStSize);
    memset(local_Comm_Lib_Conf_Struct, 0x00, nStSize);

    *Pre_Comm_Lib_Conf_Struct = local_Comm_Lib_Conf_Struct;

    return local_item_count;
}

/*
	Configure 해재 루틴 : 총 카운트를 구하고 메모리를 할당한다.
*/
void func_conf_lib_free(struct _FUNC_CONF_LIB_config_struct **Pre_Comm_Lib_Conf_Struct)
{
    struct _FUNC_CONF_LIB_config_struct	*local_Comm_Lib_Conf_Struct = *Pre_Comm_Lib_Conf_Struct;

    if (local_Comm_Lib_Conf_Struct != NULL)
    {
        free(local_Comm_Lib_Conf_Struct);
        local_Comm_Lib_Conf_Struct = NULL;

        *Pre_Comm_Lib_Conf_Struct = local_Comm_Lib_Conf_Struct;
    }

    return;
}

int	func_conf_lib_Test_Parser(char *Pre_Configure_Path_File_Name)
{
    FILE	*local_fp = NULL;

    int		local_line_count = 0;

    char	*local_line_buff_strings = NULL;
    char	local_buff = 0x00;

    int		local_line_number = 0;
    int		local_buff_locate = 0;

    char	*local_ptr_strings = NULL;

    int		local_line_safe_count = 0;

    int		local_item_count = 0;


    int		local_cf_group_count = 0;

    int		local_cf_str_copy_count = -1;

    int		local_return = 0;
    int		local_current_count = 0;


    if ((local_line_count = func_conf_lib_GetLine_Count(Pre_Configure_Path_File_Name)) == -1)
    {
        return -2;
    }

    if (3000 < local_line_count)
    {
        return -3;
    }

    if ((local_fp = fopen(Pre_Configure_Path_File_Name, "r")) == NULL)
    {
        return -4;
    }

    size_t nBufSize = (size_t)(local_line_count + 1) * 256;
    local_line_buff_strings = (char *)malloc(nBufSize);
    memset(local_line_buff_strings, 0x00, nBufSize);

    while (!feof(local_fp))
    {
        local_buff = (char)fgetc(local_fp);

        if (local_buff == EOF)
        {
            break;
        }

        if (local_buff != 0x0a)
        {
            if (local_line_safe_count < 256)
            {
                if (local_buff != 0x0d)
                {
                    local_line_buff_strings[local_buff_locate] = local_buff;
                    local_buff_locate++;
                    local_line_safe_count++;
                }
            }
        }
        else
        {
            local_line_number++;
            local_buff_locate = 256 * local_line_number;
            local_line_safe_count = 0;
        }

    } /* while */

    while (local_item_count <= local_line_number)
    {
        local_ptr_strings = &local_line_buff_strings[(local_item_count*256)];

        if (func_conf_lib_Get_Comment(local_ptr_strings) == 0)
        {
            func_conf_lib_Cut_Comment(local_ptr_strings);

            if (func_conf_lib_Chk_Item(local_ptr_strings) == 1)
            {
                local_cf_group_count++;
                local_cf_str_copy_count++;
                local_current_count++;
            }
            else
            {
                if (local_cf_str_copy_count < 0 || local_cf_group_count <= 0)
                {
                    local_return = -6;
                    break;
                }

                local_cf_str_copy_count++;
                local_current_count++;
            }
        }

        local_item_count++;
    }

    if (local_line_buff_strings != NULL)
    {
        free(local_line_buff_strings);
    }

    if (local_fp != NULL)
    {
        fclose(local_fp);
    }

    if (local_return >= 0)
    {
        local_return = local_current_count;
    }

    return local_return;
}

int	func_conf_lib_Load_Configure(char *Pre_Configure_Path_File_Name, struct _FUNC_CONF_LIB_config_struct *Pre_Comm_Lib_Conf_Struct)
{
    int		local_item_count = 0;
    int		local_chk_return = 0;

    struct _FUNC_CONF_LIB_config_struct	*local_Comm_Lib_Conf_Struct = Pre_Comm_Lib_Conf_Struct;

    if (local_Comm_Lib_Conf_Struct == NULL)
    {
        return -1;
    }

    if ((local_item_count = func_conf_lib_Test_Parser(Pre_Configure_Path_File_Name)) <= 0)
    {
        return -2;
    }

    if ((local_chk_return = func_conf_lib_Load_Parser(Pre_Configure_Path_File_Name, local_Comm_Lib_Conf_Struct)) < 0)
    {
        return -3;
    }
    else
    {
        local_chk_return = local_item_count;
    }

    if (func_conf_lib_Duplication_Check(local_Comm_Lib_Conf_Struct, local_item_count) < 0)
    {
        return -4;
    }

    return local_chk_return;
}

int func_conf_lib_Duplication_Check(struct _FUNC_CONF_LIB_config_struct *Pre_Comm_Lib_Conf_Struct, int Pre_Max_ItemCount)
{
    int		local_chk_return = 0;
    int		local_max_item_count = Pre_Max_ItemCount;

    struct _FUNC_CONF_LIB_config_struct	*local_Comm_Lib_Conf_Struct = Pre_Comm_Lib_Conf_Struct;

    int		local_count = 0;	/* 항상 0 부터 시작하는 카운터 */
    int		local_count_sub = 0;	/* 항상 0 부터 시작하는 카운터 */
    int		local_sub_break = 0;

    /* 그룹명 중복 테스트 */

    local_count = 0;
    local_count_sub = 0;

    while (local_count < local_max_item_count)
    {
        /* 그룹명만 검색한다 */
        if (local_Comm_Lib_Conf_Struct[local_count]._cf_flag == 0x01)
        {
            local_count_sub = local_count + 1;
            while (local_count_sub < local_max_item_count)
            {
                if (local_Comm_Lib_Conf_Struct[local_count]._cf_flag == 0x01 && \
					(strcmp(local_Comm_Lib_Conf_Struct[local_count]._cf_item, local_Comm_Lib_Conf_Struct[local_count_sub]._cf_item) == 0))
                {
                    local_sub_break = 1;
                    break;
                }
                local_count_sub++;
            }

            if (local_sub_break == 1)
            {
                /* 중복 발견 */
                local_chk_return = -1;
                break;
            }
        }

        local_count++;
    }

    if (local_chk_return < 0)
    {
        fprintf(stderr, "Err : Line(%d) Duplication Configure Group Name : %s is Line number (%d).\n"	\
			, local_Comm_Lib_Conf_Struct[local_count]._cf_num  \
			, local_Comm_Lib_Conf_Struct[local_count]._cf_item \
			, local_Comm_Lib_Conf_Struct[local_count_sub]._cf_num);
        return local_chk_return;
    }

    /* ITEM 중복 테스트 : 단 , 다른 그룹명의 중복은 허용되어야 한다.  A그룹의 TEST 와 B 그룹의 TEST 항목은 서로 다른것임. */

    local_count = 0;
    local_count_sub = 0;

    while (local_count < local_max_item_count)
    {
        /* ITEM 만 검색한다 */
        if (local_Comm_Lib_Conf_Struct[local_count]._cf_flag == 0x02)
        {
            local_count_sub = local_count + 1;
            while (local_count_sub < local_max_item_count)
            {
                if (local_Comm_Lib_Conf_Struct[local_count]._cf_flag == 0x02 && \
					(strcmp(local_Comm_Lib_Conf_Struct[local_count]._cf_item, local_Comm_Lib_Conf_Struct[local_count_sub]._cf_item) == 0) \
					&&	local_Comm_Lib_Conf_Struct[local_count]._cf_group == local_Comm_Lib_Conf_Struct[local_count_sub]._cf_group)
                {
                    local_sub_break = 1;
                    break;
                }
                local_count_sub++;
            }

            if (local_sub_break == 1)
            {
                /* 중복 발견 */
                local_chk_return = -1;
                break;
            }
        }

        local_count++;
    }

    if (local_chk_return < 0)
    {
        fprintf(stderr, "Err : Line(%d) Duplication Configure Item Name : %s is Line number (%d).\n"	\
			, local_Comm_Lib_Conf_Struct[local_count]._cf_num  \
			, local_Comm_Lib_Conf_Struct[local_count]._cf_item \
			, local_Comm_Lib_Conf_Struct[local_count_sub]._cf_num);
        return local_chk_return;
    }

    return  local_chk_return;
}

int	func_conf_lib_Load_Parser(char *Pre_Configure_Path_File_Name, struct _FUNC_CONF_LIB_config_struct	*Pre_Comm_Lib_Conf_Struct)
{
    FILE	*local_fp = NULL;

    /* 초기화 부분 */
    int		local_line_count = 0;

    char	*local_line_buff_strings = NULL;
    char	local_buff = 0x00;

    int		local_line_number = 0;
    int		local_buff_locate = 0;

    char	*local_ptr_strings = NULL;

    int		local_line_safe_count = 0;	/* 최대값 이 넘지 않도록 지켜 주는 카운터 */

    int		local_item_count = 0;


    int		local_cf_group_count = 0;	/* 그룹 카운트  , 0 부터 시작 */

    int		local_cf_str_copy_count = -1; /* 그룹 복사갯수 , 0 부터 시작 */

    int		local_return = 0;

    struct _FUNC_CONF_LIB_config_struct	*local_Comm_Lib_Conf_Struct = NULL;

    local_Comm_Lib_Conf_Struct = Pre_Comm_Lib_Conf_Struct;
    /* 메모리 로드 부분 */


    /* 초기화 단계 부분 */

    if (local_Comm_Lib_Conf_Struct == NULL)
    {
        return -1;
    }

    if ((local_line_count = func_conf_lib_GetLine_Count(Pre_Configure_Path_File_Name)) == -1)
    {
        return -2;
    }

    if (3000 < local_line_count)
    {
        return -3;
    }

    if ((local_fp = fopen(Pre_Configure_Path_File_Name, "r")) == NULL)
    {
        return -4;
    }

    size_t nBufSize = (size_t)(local_line_count + 1) * 256;
    local_line_buff_strings = (char *)malloc(nBufSize);
    memset(local_line_buff_strings, 0x00, nBufSize);

    /* 파일 읽기 -> 메모리 저장 */

    while (!feof(local_fp))
    {
        local_buff = (char)fgetc(local_fp);
        if (local_buff == EOF)
        {
            break;
        }

        if (local_buff != 0x0a)
        {
            if (local_line_safe_count < 256)
            {
                /* 윈도우즈 엔터값이 아닐경우만 저장한다 */
                if (local_buff != 0x0d)
                {
                    local_line_buff_strings[local_buff_locate] = local_buff;
                    local_buff_locate++;
                    local_line_safe_count++;
                }
            }
        }
        else
        {
            /* 라인번호 증가하기 */
            local_line_number++;
            local_buff_locate = 256 * local_line_number;
            local_line_safe_count = 0;
        }

    } /* while */

    while (local_item_count <= local_line_number)
    {
        local_ptr_strings = &local_line_buff_strings[(local_item_count*256)];

        if (func_conf_lib_Get_Comment(local_ptr_strings) == 0)
        {
            /* 호출한 곳으로 메모리로 전달하기 */
            func_conf_lib_Cut_Comment(local_ptr_strings);

            if (func_conf_lib_Chk_Item(local_ptr_strings) == 1)
            {
                if (254 < local_cf_group_count)
                {
                    local_return = -6;
                    break;
                }
                /* 신규 그룹 추가 */
                local_cf_group_count++;
                local_cf_str_copy_count++;

                local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_flag = 0x01;

                local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_group = local_cf_group_count;

                func_conf_lib_GItem_Copy(local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_item, local_ptr_strings);
                local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_opt[0] = 0x00;
                local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_num = local_item_count + 1;
            }
            else
            {
                if (local_cf_str_copy_count < 0 || local_cf_group_count <= 0)
                {
                    local_return = -7;
                    break;
                }
                local_cf_str_copy_count++;

                local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_flag = 0x02;
                local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_group = local_cf_group_count;

                func_conf_lib_Opt_Copy(local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_item, local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_opt, local_ptr_strings);

                local_Comm_Lib_Conf_Struct[local_cf_str_copy_count]._cf_num = local_item_count + 1;

            }
        } /* Comment 가 아닌경우만 */

        local_item_count++;
    }

    /* 메모리 및 파일 디스크립션 해제 하기 */

    if (local_line_buff_strings != NULL)
    {
        free(local_line_buff_strings);
    }

    if (local_fp != NULL)
    {
        fclose(local_fp);
    }

    return local_return;
}

int	func_conf_lib_Get_Comment(char *Pre_Strings)
{
    int		local_count = 0;
    int		local_max_count = (int)strlen(Pre_Strings);

    if (local_max_count <= 0)
    {
        return 1;
    }

    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] != 0x20 && Pre_Strings[local_count] != 0x09)
        {
            if (Pre_Strings[local_count] == '#')
            {
                return 1;
            }
            return 0;
        }

        local_count++;
    }

    return 0;
}

int	func_conf_lib_GetLine_Count(char *Pre_Configure_Path_File_Name)
{
    FILE	*local_fp = NULL;
    char	local_buff = 0x00;

    int		local_line_count = 0;

    if ((local_fp = fopen(Pre_Configure_Path_File_Name, "r")) == NULL)
    {
        return -1;
    }

    while (!feof(local_fp))
    {
        local_buff = (char)fgetc(local_fp);

        if (local_buff == EOF)
        {
            local_line_count++;
            break;
        }

        if (local_buff == 0x0a)
        {
            local_line_count++;
        }
    }

    fclose(local_fp);

    return local_line_count;
}

void func_conf_lib_Cut_Comment(char *Pre_Strings)
{
    int		local_count = 0;
    int		local_max_count = (int)strlen(Pre_Strings);

    if (local_max_count <= 0)
    {
        return;
    }

    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] == '#')
        {
            Pre_Strings[local_count] = 0x00;
            break;
        }
        local_count++;
    }

    local_count = (int)strlen(Pre_Strings);

    while (local_count >= 0)
    {
        if (Pre_Strings[local_count] != 0x20 && Pre_Strings[local_count] != 0x09 && Pre_Strings[local_count] != 0x00)
        {
            Pre_Strings[local_count + 1] = 0x00;
            break;
        }
        local_count--;
    }

    return;
}

int	func_conf_lib_Chk_Item(char *Pre_Strings)
{
    int		local_count = 0;
    int		local_max_count = (int)strlen(Pre_Strings);

    if (local_max_count <= 0)
    {
        return 1;
    }

    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] != 0x20 && Pre_Strings[local_count] != 0x09)
        {
            if (Pre_Strings[local_count] == '*')
            {
                return 1;
            }
            return 0;
        }

        local_count++;
    }

    return 0;
}
int	func_conf_lib_GItem_Copy(char *Pre_GroupNmae, char *Pre_Strings)
{
    int		local_count = 0;
    int		local_gitem_count = 0;
    int		local_max_count = (int)strlen(Pre_Strings);

    if (local_max_count <= 0)
    {
        return 1;
    }

    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] != 0x20 && Pre_Strings[local_count] != 0x09)
        {
            if (Pre_Strings[local_count] == '*')
            {
                local_count++;
                break;
            }
            break;
        }

        local_count++;
    }

    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] == 0x20 || Pre_Strings[local_count] == 0x09 || Pre_Strings[local_count] == 0x00)
        {
            break;
        }

        Pre_GroupNmae[local_gitem_count] = Pre_Strings[local_count];
        local_gitem_count++;
        local_count++;
    }

    Pre_GroupNmae[local_gitem_count] = 0x00;

    return 0;
}

int	func_conf_lib_Opt_Copy(char *Pre_ItemName, char *Pre_OptName, char *Pre_Strings)
{
    int		local_count = 0;
    int		local_gitem_count = 0;
    int		local_max_count = (int)strlen(Pre_Strings);

    if (local_max_count <= 0)
    {
        return 1;
    }

    local_gitem_count = 0;

    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] == 0x20 || Pre_Strings[local_count] == 0x09 || Pre_Strings[local_count] == 0x00)
        {
            break;
        }

        Pre_ItemName[local_gitem_count] = Pre_Strings[local_count];
        local_gitem_count++;
        local_count++;
    }

    Pre_ItemName[local_gitem_count] = 0x00;


    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] != 0x20 && Pre_Strings[local_count] != 0x09 && Pre_Strings[local_count] != 0x00)
        {
            break;
        }

        local_count++;
    }

    local_gitem_count = 0;

    while (local_count < local_max_count)
    {
        if (Pre_Strings[local_count] == 0x00)
        {
            break;
        }

        Pre_OptName[local_gitem_count] = Pre_Strings[local_count];
        local_gitem_count++;
        local_count++;
    }

    Pre_OptName[local_gitem_count] = 0x00;

    return 0;
}


int ConfigGetValue(char *pre_szFilePath, char *pre_szGroupName, char *pre_szItemName, char *pre_szOpt)
{
    int		local_count = 0;
    int		local_c = 0;
    int		local_grp_id = 0;

    struct _FUNC_CONF_LIB_config_struct	*local_Comm_Lib_Conf_Struct = NULL;

    if (func_conf_lib_Init(pre_szFilePath, &local_Comm_Lib_Conf_Struct) <= 0)
    {
        return -1;
    }

    if ((local_count = func_conf_lib_Load_Configure(pre_szFilePath, local_Comm_Lib_Conf_Struct)) <= 0)
    {
        return -2;
    }

    while (local_c < local_count)
    {
        if (local_Comm_Lib_Conf_Struct[local_c]._cf_flag == 0x01 \
			&& (strcmp(local_Comm_Lib_Conf_Struct[local_c]._cf_item, pre_szGroupName) == 0))
        {
            /* 그룹번호를 얻는다 */
            local_grp_id = local_Comm_Lib_Conf_Struct[local_c]._cf_group;
            break;
        }

        local_c++;
    }

    /* 그룹을 찾을 수 없으므로 데이타가 없음 */
    if (local_grp_id <= 0)
    {
        func_conf_lib_free(&local_Comm_Lib_Conf_Struct);

        return -1;
    }

    local_c = 0;

    while (local_c < local_count)
    {
        if (local_Comm_Lib_Conf_Struct[local_c]._cf_flag == 0x02	\
			&& (strcmp(local_Comm_Lib_Conf_Struct[local_c]._cf_item, pre_szItemName) == 0)	\
			&& local_Comm_Lib_Conf_Struct[local_c]._cf_group == local_grp_id)	\
		{
            /* 아이템을 찾았으므로 , 복사한다. */
            strcpy(pre_szOpt, local_Comm_Lib_Conf_Struct[local_c]._cf_opt);

            func_conf_lib_free(&local_Comm_Lib_Conf_Struct);

            return 0;
        }

        local_c++;
    }

    func_conf_lib_free(&local_Comm_Lib_Conf_Struct);

    return -2;
}