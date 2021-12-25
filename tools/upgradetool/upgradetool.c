#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <openssl/md5.h>
#include <endian.h>

#include "kernel.h"
#include "cJSON.h"

#ifdef YYTEK
#include <nvm.h>
#include <fw_env.h>
#endif

#define  	pr_info(format,...) \
			printf("[  INFO]  "format"\n", ##__VA_ARGS__)
#define 	pr_err(format,...) \
			printf("[ ERROR]  %s -- %d --: "format"\n", __func__, __LINE__, ##__VA_ARGS__)

#define 	BUF_SIZE        1024*1024
#define 	READ_MAX_SIZE   1024*1024
#define 	MD5_SIZE        16
#define 	MD5_STR_LEN     (MD5_SIZE * 2)

cJSON *root_js = NULL;
cJSON *all_files_arr;
cJSON *all_partitions_arr;

char buf[BUF_SIZE];

#ifdef YYTEK
int is_sbc_or_som_matched(int sbc, struct nvm *nvm, int pcba, int tag)
{
	pr_info("Current: nvm[0].type=0x%02x nvm[0].code=0x%04x", nvm->type, htole16(nvm->code));
	pr_info("Current %s pcba version: 0x%x Given : 0x%x", sbc ? "SBC" : "SOM", nvm->pcba, pcba);
	pr_info("Current %s tag : 0x%x Given : 0x%x", sbc ? "SBC" : "SOM" , nvm->tag, tag);

	if (pcba != -1 && tag != -1)
		return !(nvm->pcba == pcba && nvm->tag == tag);
	else if (pcba == -1 && tag != -1)
		return !(nvm->tag == tag);
	else if (tag == -1 && pcba != -1)
		return !(nvm->pcba == pcba);

	return 0;
}

int is_cb_board_matched(cJSON *arr_cb_board, struct nvm *nvm)
{
	int rc = -EINVAL;
	cJSON *cb_board_item = NULL;
	char *cb_board_type_js = NULL, *cb_board_code_js = NULL;
	char *cb_board_name = NULL;
	int cb_tag, cb_pcba;

	cJSON_ArrayForEach(cb_board_item, arr_cb_board) {
		cb_board_name = cJSON_GetObjectItemCaseSensitive(cb_board_item, "board_type")->valuestring;
		cb_board_type_js = strtok(cb_board_name, "-");
		cb_board_code_js = strtok(NULL, "-");
		cb_pcba = cJSON_GetObjectItemCaseSensitive(cb_board_item, "pcba") \
				? cJSON_GetObjectItemCaseSensitive(cb_board_item, "pcba")->valueint : -1;
		cb_tag = cJSON_GetObjectItemCaseSensitive(cb_board_item, "tag") \
				? cJSON_GetObjectItemCaseSensitive(cb_board_item, "tag")->valueint : -1;

		if (htole16(nvm->code) == strtol(cb_board_code_js, NULL, 16) \
				&& type_is_cb(nvm) \
				&& (!strcmp(cb_board_type_js, "302") || !strcmp(cb_board_type_js, "CB"))) {

			pr_info("Current CB Board: %s-%04x, already supported", cb_board_type_js, htole16(nvm->code));
			pr_info("Read CB pcba version: %x Given : %x", nvm->pcba, cb_pcba);
			pr_info("Read CB tag : 0x%x Given : 0x%x", nvm->tag, cb_tag);

			if (cb_pcba != -1 && cb_tag != -1)
				return !(nvm->pcba == cb_pcba && nvm->tag == cb_tag);
			else if (cb_pcba == -1 && cb_tag != -1)
				return !(nvm->tag == cb_tag);
			else if (cb_tag == -1 && cb_pcba != -1)
				return !(nvm->pcba == cb_pcba);
			rc = 0;
		}
	}

	if (rc)
		pr_err("Not found supported cb board.");

	return rc;
}

int check_board_type(void)
{
	struct nvm nvm[2];
	int tag, pcba, rc, nvm_num;
	char *board_type_js = NULL, *board_code_js = NULL;
	char *board_name = NULL;
	cJSON *arr_supported_product = NULL, *arr_cb_board = NULL;
	cJSON *arr_product_item = NULL;

	rc = yy_read_nvm(nvm, &nvm_num);
	if (rc) {
		pr_err("Read eeprom failed");
		return rc;
	}

	arr_supported_product = cJSON_GetObjectItem(root_js, "supported_product");
	cJSON_ArrayForEach(arr_product_item, arr_supported_product) {
		board_name = cJSON_GetObjectItemCaseSensitive(arr_product_item, "board_type")->valuestring;
		board_type_js = strtok(board_name, "-");
		board_code_js = strtok(NULL, "-");
		pcba = cJSON_GetObjectItemCaseSensitive(arr_product_item, "pcba") \
				? cJSON_GetObjectItemCaseSensitive(arr_product_item, "pcba")->valueint : -1;
		tag = cJSON_GetObjectItemCaseSensitive(arr_product_item, "tag") \
				? cJSON_GetObjectItemCaseSensitive(arr_product_item, "tag")->valueint : -1;

		if (type_is_sbc(&nvm[0]) \
				&& htole16(nvm[0].code) == strtol(board_code_js, NULL, 16) \
				&& (!strcmp(board_type_js, "301") || !strcmp(board_type_js, "SBC"))) {
			/* SBC board*/
			pr_info("Current SBC Board: %s-%04x, already supported", board_type_js, htole16(nvm[0].code));
			return is_sbc_or_som_matched(1, &nvm[0], pcba, tag);
		} else if (type_is_som(&nvm[0])
				&& htole16(nvm[0].code) == strtol(board_code_js, NULL, 16) \
				&& (!strcmp(board_type_js, "303") || !strcmp(board_type_js, "SOM"))) {
			/* SOM board*/
			pr_info("Current SOM Board: %s-%04x, already supported", board_type_js, htole16(nvm[0].code));
			if (!is_sbc_or_som_matched(0, &nvm[0], pcba, tag)) {
				/* need a cb board match*/
				arr_cb_board = cJSON_GetObjectItemCaseSensitive(arr_product_item, "carriers");
				if (arr_cb_board == NULL) {
					pr_err("Not a cb board json config found");
					return -EINVAL;
				}

				return type_is_cb(&nvm[1]) ? is_cb_board_matched(arr_cb_board, &nvm[1]) : -EINVAL;
			} else
				return -EINVAL;
		}
	}

	return rc;
}
#else

int check_board_type(void)
{

	return 0;
}
#endif

int check_image_json(const char *json)
{
	struct stat js_stat;
	FILE* fp = fopen(json, "r");

	if (access(json, R_OK)) {
		pr_err("Image json file not found!");
		return -EINVAL;
	}

	pr_info("Find json file: %s", json);

	stat(json, &js_stat);

	if (fp == NULL) {
		pr_err("Image json open failed!");
		return -EINVAL;
	}
	fread(buf, 1, js_stat.st_size, fp);

	root_js = cJSON_Parse(buf);
	if (root_js == NULL) {
		pr_err("Json parse failed! Maybe json syntax error");
		return -EINVAL;
	}
	pr_info("Json file parse success");

	fclose(fp);

	return 0;
}

int get_file_md5(const char *file,  char *md5_str)
{
	FILE *fp;
	struct stat js_stat;
	int ret = 0, i = 0, num=0;
	MD5_CTX md5;
	uint8_t data[READ_MAX_SIZE];
	uint8_t md5_value[MD5_SIZE];

	stat(file, &js_stat);

	if ((fp = fopen(file, "r")) == NULL) {
		pr_err("Open %s failed", file);
		return -1;
	}

	num = 0;
	MD5_Init(&md5);

	while (num < js_stat.st_size) {
		ret = fread(data, 1, READ_MAX_SIZE, fp);
		if (ret == -1) {
			pr_err("Read %s failed,%m!", file);
			fclose(fp);
			return -EINVAL;
		}
		num += ret;
		MD5_Update(&md5, data, ret);
		if (ret == 0) {
			break;
		}
	}
	fclose(fp);

	MD5_Final((unsigned char *)&md5_value, &md5);

	for (i = 0; i < MD5_SIZE; i++) {
		snprintf(md5_str + i * 2, 2 + 1, "%02x", md5_value[i]);
	}
	md5_str[MD5_STR_LEN] = '\0';

	return 0;
}

int check_file_ok(char *file, char *hash)
{
	char md5_val[MD5_STR_LEN + 1];

	if (access(file, R_OK)) {
		pr_err("%s not found!\n", file);
		return -EINVAL;
	}

	get_file_md5(file, md5_val);

	if (strcmp(md5_val, hash)) {
		pr_err("file: %s \n  get md5:[%s] \ngaven md5:[%s]\n",file, md5_val, hash);
		return -EINVAL;
	} else {
		pr_info("Checking file %s md5sum is ok",file);
	}

	return 0;
}

int add_this_file_to_array(char *storages_name, cJSON *arr_file_item, char *device, int part_index)
{
	/** add all files to a array*/
	char *file_name = NULL, path[1024] = { 0 };
	char *options = NULL, *dir = NULL;
	cJSON *temp_fileobj = NULL;

	file_name = cJSON_GetObjectItemCaseSensitive(arr_file_item, "filename")->valuestring;

	cJSON_AddItemToArray(all_files_arr, temp_fileobj = cJSON_CreateObject());
	cJSON_AddItemToObject(temp_fileobj, "filename", cJSON_CreateString(file_name));
	cJSON_AddItemToObject(temp_fileobj, "device", cJSON_CreateString(device));

	if (!strcmp(storages_name, "boot")) {
		options = cJSON_GetObjectItemCaseSensitive(arr_file_item, "options")\
				? cJSON_GetObjectItemCaseSensitive(arr_file_item, "options")->valuestring : NULL;
		if (options == NULL) {
			pr_err("Not specify options of offset to burn!");
			return -EINVAL;
		}
		cJSON_AddItemToObject(temp_fileobj, "options", cJSON_CreateString(options));
		cJSON_AddItemToObject(temp_fileobj, "type", cJSON_CreateString("raw"));

	} else if (!strcmp(storages_name, "filesystem")) {
		dir = cJSON_GetObjectItemCaseSensitive(arr_file_item, "dir")\
			? cJSON_GetObjectItemCaseSensitive(arr_file_item, "dir")->valuestring : NULL;
		if (dir == NULL) {
			pr_err("Not setting %s path to burn", file_name);
			return -EINVAL;
		}
		if (!strncmp(device, "mmcblk", 6) || !strncmp(device, "nvme", 4)) {
			sprintf(path, "/run/media/%sp%d%s", device, part_index, dir);
		} else if (!strncmp(device, "sd", 2)) {
			sprintf(path, "/run/media/%s%d%s", device, part_index, dir);
		} else {
			pr_err("Unsupport block device : %s", device);
			return -EINVAL;
		}
		cJSON_AddItemToObject(temp_fileobj, "partition_num", cJSON_CreateNumber(part_index));
		cJSON_AddItemToObject(temp_fileobj, "dir", cJSON_CreateString(path));
		cJSON_AddItemToObject(temp_fileobj, "type", cJSON_CreateString("filesystem"));
	}

	return 0;
}

int check_all_image_files(void)
{
	char *storages_name = NULL, *file_name = NULL;
	char *device = NULL, *hash = NULL;
	int part_index = 0;
	cJSON *storages_arr = NULL, *files_arr = NULL, *partitions_arr = NULL;
	cJSON *arr_stor_item = NULL, *arr_file_item = NULL, *arr_parti_item = NULL;

	all_files_arr = cJSON_CreateArray();
	storages_arr = cJSON_GetObjectItem(root_js,"storages");

	cJSON_ArrayForEach(arr_stor_item, storages_arr) {
		storages_name = cJSON_GetObjectItemCaseSensitive(arr_stor_item, "name")->valuestring;
		device = cJSON_GetObjectItemCaseSensitive(arr_stor_item, "device") \
				? cJSON_GetObjectItemCaseSensitive(arr_stor_item, "device")->valuestring : NULL;
		if (device == NULL) {
			pr_err("Not specify device to burn!");
			return -EINVAL;
		}
		if (!strcmp(storages_name, "boot")) {
			files_arr = cJSON_GetObjectItemCaseSensitive(arr_stor_item, "files");
			cJSON_ArrayForEach(arr_file_item, files_arr) {
				file_name = cJSON_GetObjectItemCaseSensitive(arr_file_item, "filename")->valuestring;
				hash = cJSON_GetObjectItemCaseSensitive(arr_file_item, "hash")->valuestring;
				if (check_file_ok(file_name, hash)) {
					return -EINVAL;
				}
				add_this_file_to_array(storages_name, arr_file_item, device, 0);
			}
		} else if (!strcmp(storages_name, "filesystem")) {
			partitions_arr = cJSON_GetObjectItem(arr_stor_item, "partitions");
			cJSON_ArrayForEach(arr_parti_item, partitions_arr) {
				part_index = cJSON_GetObjectItemCaseSensitive(arr_parti_item, "partition_num")\
					? cJSON_GetObjectItemCaseSensitive(arr_parti_item, "partition_num")->valueint : -1;
				if (part_index < 0) {
					pr_err("Not setting %s which partition to burn", file_name);
					return -EINVAL;
				}
				files_arr = cJSON_GetObjectItemCaseSensitive(arr_parti_item, "files");
				cJSON_ArrayForEach(arr_file_item, files_arr) {
					file_name = cJSON_GetObjectItemCaseSensitive(arr_file_item, "filename")->valuestring;
					hash = cJSON_GetObjectItemCaseSensitive(arr_file_item, "hash")->valuestring;
					if (check_file_ok(file_name, hash))
						return -EINVAL;
					add_this_file_to_array(storages_name, arr_file_item, device, part_index);
				}
			}
		} else {
			printf("storages_type not support :%s\n", storages_name);
			return -EINVAL;
		}
	}

	return 0;
}

int do_mount_partition(char *device, int part_num)
{
	int i, ret;
	char cmd[512] = { 0 };

	sprintf(cmd, "cd / && umount `cat /proc/mounts | grep %s | awk '{print $2}'` 2> /dev/null", device);
	system("killall syslogd klogd udevd mdev 2> /dev/null");
	system(cmd);

	for (i = 1; i <= part_num; i++) {
		memset(cmd, 0, sizeof(cmd));
		if (!strncmp(device, "mmcblk", 6) || !strncmp(device, "nvme", 4)) {
			sprintf(cmd, "mkdir -p /run/media/%sp%d && mount /dev/%sp%d /run/media/%sp%d", device, i, device, i, device, i);
			ret = system(cmd);
		} else if (!strncmp(device, "sd", 2)) {
			sprintf(cmd, "mkdir -p /run/media/%s%d && mount /dev/%s%d /run/media/%s%d", device, i, device, i, device, i);
			ret = system(cmd);
		} else {
			pr_err("Unsupport block device : %s", device);
			return -EINVAL;
		}

		if (ret) {
			pr_err("Mount %sp%d failed", device, i);
			return -EINVAL;
		} else {
			if (!strncmp(device, "mmcblk", 6))
				pr_info("Mount %sp%d success at /run/media/%sp%d", device, i, device, i);
			else
				pr_info("Mount %s%d success at /run/media/%s%d", device, i, device, i);
		}
	}

	return 0;
}

int do_partition(char *part_flag)
{
	/** 2@1@16@mmcblk0@1,vfat,8G,format_true@2,ext3,4G,format_true
	 *  partition_num = 2
	 *  part_true = 1
	 *  skip_sector = 16
	 *  device = mmcblk0
	 *  partition 1: vfat,8G,format_true
	 *  Partition 2: ext3,4G,format_true
	 */

	int ret = 0;
	char cmd[512] = { 0 };

	pr_info("Start Partition, flags = %s", part_flag);

	sprintf(cmd, "fdisk-part.sh %s", part_flag);
	ret = system(cmd);
	if (ret == 0) {
		pr_info("Partition success");
	} else {
		pr_info("Partition failed, ret=%d", ret);
		return -EINVAL;
	}

	return 0;
}

int parse_partition_js(cJSON *partitions_arr, char *device, char *part_flag, int part_true)
{
	char *fs_type = NULL, *part_size = NULL;
	cJSON *arr_parti_item = NULL;
	char part_flag_temp[64] = { 0 };
	int part_num = 0, part_index = 0, skip_sector = 16, format_true = 0;

	part_num = cJSON_GetArraySize(partitions_arr);
	sprintf(part_flag_temp,"%d@%d", part_num, part_true);
	strcat(part_flag, part_flag_temp);
	memset(part_flag_temp, 0, sizeof(part_flag_temp));

	cJSON_ArrayForEach(arr_parti_item, partitions_arr) {
		part_index = cJSON_GetObjectItemCaseSensitive(arr_parti_item, "partition_num")->valueint;
		part_size = cJSON_GetObjectItemCaseSensitive(arr_parti_item, "size") \
				? cJSON_GetObjectItemCaseSensitive(arr_parti_item, "size")->valuestring : NULL;
		fs_type = cJSON_GetObjectItemCaseSensitive(arr_parti_item, "filesystem") \
				? cJSON_GetObjectItemCaseSensitive(arr_parti_item, "filesystem")->valuestring : NULL;
		format_true = cJSON_GetObjectItemCaseSensitive(arr_parti_item, "format") \
				? cJSON_GetObjectItemCaseSensitive(arr_parti_item, "format")->valueint : 0;

		if (part_index == 1) {
			/* mmcblk0 default skip 16 sectors, sda harddisk device default skip 63 sectors */
			skip_sector = strncmp(device, "mmc", 3) ? 63 : 16;
			skip_sector = cJSON_GetObjectItemCaseSensitive(arr_parti_item, "skipsector") \
						? cJSON_GetObjectItemCaseSensitive(arr_parti_item, "skipsector")->valueint : skip_sector;
			sprintf(part_flag_temp,"@%d@%s", skip_sector, device);
			strcat(part_flag, part_flag_temp);
			memset(part_flag_temp, 0, sizeof(part_flag_temp));
		}

		if (part_true == 1 && part_index < part_num && part_size == NULL) {
			pr_err("partiton %d's size not set, will exit", part_index);
			return -EINVAL;
		}
		if ((part_true == 1 || format_true == 1) && fs_type == NULL) {
			pr_err("partiton %d's fs_type not set, will exit", part_index);
			return -EINVAL;
		}
		if (strcmp(fs_type, "vfat") && strcmp(fs_type, "ext3") && strcmp(fs_type, "ext4") && strcmp(fs_type, "ext2")) {
			pr_err("fs_type : %s not support, will exit", fs_type);
			return -EINVAL;
		}
		sprintf(part_flag_temp, "@%d,%s,%s,%s", part_index, \
							fs_type != NULL ? fs_type : "none", \
							part_size != NULL ? part_size : "none", \
							format_true == 1 ? "format_true" : "format_false");
		strcat(part_flag, part_flag_temp);
		memset(part_flag_temp, 0, sizeof(part_flag_temp));
	}

	return 0;
}

int check_disk_do_partition(void)
{
	char *storages_name = NULL;
	char *device = NULL;
	char part_flag[128] = { 0 };
	int parti_arrsize = 0, part_true = 0;
	cJSON *storages_arr = NULL, *partitions_arr = NULL, *arr_stor_item = NULL;

	storages_arr = cJSON_GetObjectItem(root_js, "storages");
	cJSON_ArrayForEach(arr_stor_item, storages_arr) {
		storages_name = cJSON_GetObjectItemCaseSensitive(arr_stor_item, "name")->valuestring;
			if (!strcmp(storages_name, "filesystem")) {
				device = cJSON_GetObjectItemCaseSensitive(arr_stor_item, "device")\
					? cJSON_GetObjectItemCaseSensitive(arr_stor_item, "device")->valuestring : NULL;
				if (device == NULL) {
					pr_err("device not set, will exit");
					return -EINVAL;
				}

				partitions_arr = cJSON_GetObjectItem(arr_stor_item, "partitions");
				parti_arrsize = cJSON_GetArraySize(partitions_arr);
				part_true = cJSON_GetObjectItemCaseSensitive(arr_stor_item, "parted") \
					? cJSON_GetObjectItemCaseSensitive(arr_stor_item, "parted")->valueint : 0;

				parse_partition_js(partitions_arr, device, part_flag, part_true);

				if (do_partition(part_flag))
					return -EINVAL;
				do_mount_partition(device, parti_arrsize);
		}
	}

	return 0;
}

static int flash_erase(int fd, struct mtd_info_user *mtd, int offset, int size)
{
	struct erase_info_user erase;

	erase.start = offset;
	erase.length = (size + mtd->erasesize - 1) / mtd->erasesize;
	erase.length *= mtd->erasesize;

	return ioctl(fd, MEMERASE, &erase) ? errno : 0;
}

int update_flash(const char *mtd_dev, int offset, void *buf, int size)
{
	int rc, fd;
	int erasesize, step;
	char *src;
	struct mtd_info_user mtd;

	fd = open(mtd_dev, O_SYNC | O_RDWR);
	if (fd < 0) {
		pr_err("Open device %s failed:%m!", mtd_dev);
		return errno;
	}

	rc = ioctl(fd, MEMGETINFO, &mtd);
	if (rc < 0) {
		pr_err("Ioctl failed %m!");
		rc = errno;
		goto err;
	}

	if ((offset + size) > mtd.size) {
		pr_err("Offset + file size > mtd's size");
		rc = EFBIG;
		goto err;
	}

	erasesize = mtd.erasesize;
	step = erasesize;

	src = malloc(erasesize);
	if (!src) {
		pr_err("Malloc failed");
		rc = ENOMEM;
		goto err;
	}

	while (size) {
		if (size < erasesize)
			step = size;

		lseek(fd, offset, SEEK_SET);
		rc = read(fd, src, step);
		if (rc != step) {
			rc = EFAULT;
			goto err_buf;
		}

		if (memcmp(src, buf, step)) {
			rc = flash_erase(fd, &mtd, offset, step);
			if (rc) {
				pr_err("flash_erase failed");
				goto err_buf;
			}

			lseek(fd, offset, SEEK_SET);

			rc = write(fd, buf, step);
			if (rc != step) {
				rc = EFAULT;
				goto err_buf;
			}
		}
		buf += step;
		offset += step;
		size -= step;
	}

	rc = 0;
err_buf:
	free(src);
err:
	close(fd);

	return rc;
}

int burn_to_raw_mtd(const char *file, const char *mtd_device, int offset)
{
	int fd;
	int rc, size;
	char cmd[512] = { 0 };
	struct stat filestat;
	unsigned char *buf = NULL;

	pr_info("Burn %s to %s at offset 0x%x bytes",file, mtd_device, offset);

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		pr_err("%s - %m!", file);
		return errno;
	}

	rc = fstat(fd, &filestat);
	if (rc < 0) {
		pr_err("%s - %m!", file);
		rc = errno;
		goto err;
	}

#if defined(CONFIG_SOC_PHYTIUM)
	size = ALIGN(filestat.st_size, 8);
#else
	size = filestat.st_size;
#endif
	buf = calloc(1, size);
	rc = read(fd, buf, size);
	if (rc != filestat.st_size) {
		rc = EFAULT;
		goto err_free;
	}

	if (!strncmp(mtd_device, "/dev/mmcblk", 10)) {
		if (offset % 512)
			sprintf(cmd, "dd if=%s of=%s bs=1 seek=%d > /dev/null 2>&1", file, mtd_device, offset);
		else
			sprintf(cmd, "dd if=%s of=%s bs=512 seek=%d > /dev/null 2>&1", file, mtd_device, offset/512);
		rc = system(cmd);
		if (rc)
			pr_err("Update %s failed, rc=%d", file, rc);
		else
			pr_info("Update %s success", file);
	} else {
		/** spi flash mtd device*/
		rc = update_flash(mtd_device, offset, buf, size);
		if (rc)
			pr_err("Update %s failed, rc=%d", file, rc);
		else
			pr_info("Update %s success", file);
	}

err_free:
	free(buf);
err:
	close(fd);

	return rc;
}

int copy_to_filesystem(char *file, const char *path)
{
	char cmd[1024] = { 0 };
	char *token = NULL;

	token = strrchr(file, '.');
	if (!strcmp(token, ".tgz") || !strcmp(token, ".gz") || !strcmp(token, ".bz2"))
		sprintf(cmd, "tar xmf %s -C %s && sync", file, path); /* need to extract file*/
	else
		sprintf(cmd, "mkdir -p %s && cp %s %s -ad && sync", path, file, path);

	pr_info("%s", cmd);

	return system(cmd);
}

int do_burn_files(void)
{
	char dev[64] = { 0 };
	int seek = 0;
	cJSON *arr_file_item = NULL;
	char *file_name = NULL, *options = NULL, *device = NULL;
	char *path_partition = NULL, *token = NULL;

	/** burn raw or extract tar file first*/
	cJSON_ArrayForEach(arr_file_item, all_files_arr) {
		file_name = cJSON_GetObjectItemCaseSensitive(arr_file_item, "filename")->valuestring;
		device = cJSON_GetObjectItemCaseSensitive(arr_file_item, "device")->valuestring;
		if (!strcmp("raw", cJSON_GetObjectItemCaseSensitive(arr_file_item, "type")->valuestring)) {
			options = cJSON_GetObjectItemCaseSensitive(arr_file_item, "options")->valuestring;
			token = strrchr(options, '=');
			seek = strtoul(token + 1, NULL, 0);
			sprintf(dev, "/dev/%s", device);
			if (burn_to_raw_mtd(file_name, dev, seek)) {
				pr_err("Burn %s failed", file_name);
				return -EINVAL;
			}
		} else if (!strcmp("filesystem", cJSON_GetObjectItemCaseSensitive(arr_file_item, "type")->valuestring)) {
			path_partition = cJSON_GetObjectItemCaseSensitive(arr_file_item, "dir")->valuestring;
			token = strrchr(file_name, '.');
			if (!strcmp(token, ".tgz") || !strcmp(token, ".gz") || !strcmp(token, ".bz2")) {
				if (copy_to_filesystem(file_name, path_partition)) {
					pr_err("Burn %s failed", file_name);
					return -EINVAL;
				}
			}
		} else {
			pr_err("Not support type :%s", cJSON_GetObjectItemCaseSensitive(arr_file_item,"type")->valuestring);
		}
	}

	/** copy files later*/
	cJSON_ArrayForEach(arr_file_item, all_files_arr) {
		file_name = cJSON_GetObjectItemCaseSensitive(arr_file_item,"filename")->valuestring;
		if (!strcmp("filesystem", cJSON_GetObjectItemCaseSensitive(arr_file_item,"type")->valuestring)) {
			path_partition = cJSON_GetObjectItemCaseSensitive(arr_file_item,"dir")->valuestring;
			token = strrchr(file_name, '.');
			if (strcmp(token, ".tgz") && strcmp(token, ".gz") && strcmp(token, ".bz2")) {
				if (copy_to_filesystem(file_name, path_partition)) {
					pr_err("Burn %s failed", file_name);
					return -EINVAL;
				}
			}
		}
	}

	return 0;
}

int do_pre_exec(void)
{
	char cmd[512] = { 0 };
	char *cmd_temp = NULL;
	char *parameter = NULL;
	cJSON *pre_exec_obj = NULL, *pre_exec_item = NULL;

	pr_info("Checking if a pre-exec shell need to execute");

	pre_exec_obj = cJSON_GetObjectItem(root_js, "pre-exec");
	if (pre_exec_obj == NULL) {
			pr_info("Not find a pre-exec shell");
			return 0;
	}

	cJSON_ArrayForEach(pre_exec_item, pre_exec_obj) {
		cmd_temp = cJSON_GetObjectItemCaseSensitive(pre_exec_item, "cmd") \
				? cJSON_GetObjectItemCaseSensitive(pre_exec_item, "cmd")->valuestring : NULL;
		parameter = cJSON_GetObjectItemCaseSensitive(pre_exec_item, "parameter") \
				? cJSON_GetObjectItemCaseSensitive(pre_exec_item, "parameter")->valuestring : NULL;

		if (cmd_temp == NULL) {
			pr_err("pre-exec shell : %s not found!\n", cmd_temp);
			return -EINVAL;
		}

		pr_info("Executing pre-shell script");
		if (parameter)
			sprintf(cmd, "%s %s", cmd_temp, parameter);
		else
			sprintf(cmd, "%s", cmd_temp);

		pr_info("%s", cmd);
		system(cmd);
	}

	return 0;
}

int do_post_exec(void)
{
	char cmd[512] = { 0 };
	char *cmd_temp = NULL;
	char *parameter = NULL;
	cJSON *post_exec_obj = NULL, *post_exec_item = NULL;

	pr_info("Checking if a post-exec shell need to execute");

	post_exec_obj = cJSON_GetObjectItem(root_js, "post-exec");
	if (post_exec_obj == NULL) {
		pr_info("Not find a post-exec shell");
		return 0;
	}

	cJSON_ArrayForEach(post_exec_item, post_exec_obj) {
		cmd_temp = cJSON_GetObjectItemCaseSensitive(post_exec_item, "cmd") \
				? cJSON_GetObjectItemCaseSensitive(post_exec_item, "cmd")->valuestring : NULL;
		parameter = cJSON_GetObjectItemCaseSensitive(post_exec_item, "parameter") \
				? cJSON_GetObjectItemCaseSensitive(post_exec_item, "parameter")->valuestring : NULL;

		if (cmd_temp == NULL) {
			pr_err("post-exec shell : %s not found!\n", cmd_temp);
			return -EINVAL;
		}

		pr_info("Executing post-shell script");
		if (parameter)
			sprintf(cmd, "%s %s", cmd_temp, parameter);
		else
			sprintf(cmd, "%s", cmd_temp);

		pr_info("%s", cmd);
		system(cmd);
	}

	return 0;
}

int main(int argc, const char *argv[])
{
	if (argc != 2) {
		pr_err("usage: %s image.json", argv[0]);
		return -EINVAL;
	}

	if (check_image_json(argv[1])) {
		pr_err("Please check image json");
		return -EINVAL;
	}

	if (check_board_type()) {
		pr_err("Board not support!");
		return -EINVAL;
	}

	if (check_all_image_files()) {
		pr_err("Please check image files");
		return -EINVAL;
	}

	if (do_pre_exec()) {
		pr_err("pre-exec failed");
		return -EINVAL;
	}

	pr_info("All files check ok");

	if (check_disk_do_partition()) {
		pr_err("Please check json partition setting ");
		return -EINVAL;
	}

	if (do_burn_files()) {
		pr_err("Burn files failed ");
		return -EINVAL;
	}

	if (do_post_exec()) {
		pr_err("post-exec failed");
		return -EINVAL;
	}

	system("sync;sync");
	pr_info("Upgrade success, please reboot");

	cJSON_Delete(all_files_arr);
	cJSON_Delete(root_js);

	return 0;
}

