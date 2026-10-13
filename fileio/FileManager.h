#pragma once

#include "../utils/MyBitMap.h"
#include "../utils/pagedef.h"
#include "../utils/platform_io.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>

class FileManager {
private:
	std::array<std::FILE*, MAX_FILE_NUM> files{};
	MyBitMap* fm;
	MyBitMap* tm;
	int _createFile(const std::filesystem::path& name) {
		std::FILE* file = ljcdb::platform::openFile(name, "wb");
		if (file == nullptr)
			return -1;
		return std::fclose(file) == 0 ? 0 : -1;
	}
	int _openFile(const std::filesystem::path& name, int fileID) {
		std::FILE* file = ljcdb::platform::openFile(name, "r+b");
		if (file == nullptr)
			return -1;
		// Page I/O already has its own buffer pool. Avoid a second buffering
		// layer and its read/write transition rules in stdio.
		std::setvbuf(file, nullptr, _IONBF, 0);
		files[fileID] = file;
		return 0;
	}
public:
	/*
	 * FilManager构造函数
	 */
	FileManager() {
		fm = new MyBitMap(MAX_FILE_NUM, 1);
		tm = new MyBitMap(MAX_TYPE_NUM, 1);
	}
	/*
	 * @函数名writePage
	 * @参数fileID:文件id，用于区别已经打开的文件
	 * @参数pageID:文件的页号
	 * @参数buf:存储信息的缓存(4字节无符号整数数组)
	 * @参数off:偏移量
	 * 功能:将buf+off开始的2048个四字节整数(8kb信息)写入fileID和pageID指定的文件页中
	 * 返回:成功操作返回0
	 */
	int writePage(int fileID, int pageID, BufType buf, int off) {
		if (fileID < 0 || fileID >= MAX_FILE_NUM || pageID < 0 ||
			buf == nullptr || files[fileID] == nullptr)
			return -1;
		std::FILE* file = files[fileID];
		const auto offset = static_cast<std::uint64_t>(pageID) * PAGE_SIZE;
		if (!ljcdb::platform::seekFile(file, offset))
			return -1;
		BufType b = buf + off;
		return std::fwrite(static_cast<void*>(b), 1, PAGE_SIZE, file) == PAGE_SIZE
			? 0
			: -1;
	}
	/*
	 * @函数名readPage
	 * @参数fileID:文件id，用于区别已经打开的文件
	 * @参数pageID:文件页号
	 * @参数buf:存储信息的缓存(4字节无符号整数数组)
	 * @参数off:偏移量
	 * 功能:将fileID和pageID指定的文件页中2048个四字节整数(8kb)读入到buf+off开始的内存中
	 * 返回:成功操作返回0
	 */
	int readPage(int fileID, int pageID, BufType buf, int off) {
		if (fileID < 0 || fileID >= MAX_FILE_NUM || pageID < 0 ||
			buf == nullptr || files[fileID] == nullptr)
			return -1;
		std::FILE* file = files[fileID];
		const auto offset = static_cast<std::uint64_t>(pageID) * PAGE_SIZE;
		if (!ljcdb::platform::seekFile(file, offset))
			return -1;
		BufType b = buf + off;
		const std::size_t bytesRead = std::fread(static_cast<void*>(b), 1, PAGE_SIZE, file);
		if (bytesRead < PAGE_SIZE) {
			if (std::ferror(file)) {
				std::clearerr(file);
				return -1;
			}
			std::memset(reinterpret_cast<unsigned char*>(b) + bytesRead, 0,
				PAGE_SIZE - bytesRead);
			std::clearerr(file);
		}
		return 0;
	}
	/*
	 * @函数名closeFile
	 * @参数fileID:用于区别已经打开的文件
	 * 功能:关闭文件
	 * 返回:操作成功，返回0
	 */
	int closeFile(int fileID) {
		if (fileID < 0 || fileID >= MAX_FILE_NUM || files[fileID] == nullptr)
			return -1;
		std::FILE* file = files[fileID];
		files[fileID] = nullptr;
		fm->setBit(fileID, 1);
		return std::fclose(file) == 0 ? 0 : -1;
	}
	/*
	 * @函数名createFile
	 * @参数name:文件名
	 * 功能:新建name指定的文件名
	 * 返回:操作成功，返回true
	 */
	bool createFile(const std::filesystem::path& name) {
		return _createFile(name) == 0;
	}
	/*
	 * @函数名openFile
	 * @参数name:文件名
	 * @参数fileID:函数返回时，如果成功打开文件，那么为该文件分配一个id，记录在fileID中
	 * 功能:打开文件
	 * 返回:如果成功打开，在fileID中存储为该文件分配的id，返回true，否则返回false
	 */
	bool openFile(const std::filesystem::path& name, int& fileID) {
		fileID = fm->findLeftOne();
		if (fileID < 0 || fileID >= MAX_FILE_NUM || files[fileID] != nullptr ||
			_openFile(name, fileID) != 0) {
			fileID = -1;
			return false;
		}
		fm->setBit(fileID, 0);
		return true;
	}
	int newType() {
		int t = tm->findLeftOne();
		tm->setBit(t, 0);
		return t;
	}
	void closeType(int typeID) {
		tm->setBit(typeID, 1);
	}
	void shutdown() {
		for (std::FILE*& file : files) {
			if (file != nullptr) {
				std::fclose(file);
				file = nullptr;
			}
		}
		delete tm;
		delete fm;
		tm = nullptr;
		fm = nullptr;
	}
	FileManager(const FileManager&) = delete;
	FileManager& operator=(const FileManager&) = delete;
	~FileManager() {
		this->shutdown();
	}
};
