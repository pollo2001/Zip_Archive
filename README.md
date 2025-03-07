# **Zip Archive Project** 🎉

## **Overview** 🎬
The **Archive Project** is designed to implement and manage custom archive files in C++. This project allows for creating, opening, adding, extracting, removing, and listing files in a custom archive format (with a `.arc` extension). Additionally, the project includes a file compression and chunking mechanism to store data efficiently.

---

## **Features** ✨
- **Create Archives** 🗄️  
  Create new `.arc` archives for storing files.

- **Open Existing Archives** 📂  
  Open and access previously created `.arc` archives.

- **File Operations** 📑  
  - **Add**: Add files to an archive (with optional compression).
  - **Extract**: Retrieve files from an archive.
  - **Remove**: Delete files from an archive.

- **Chunking and Compression** 🔒  
  - Uses chunking to break files into 1024-byte blocks for storage.
  - Supports file compression for efficient storage.

- **Metadata Storage** 📝  
  Archive files include metadata such as the filename, file size, compression status, and timestamps.

---

## **Getting Started** 🚀

### **Prerequisites** 🛠️
To run the project, make sure you have:
- A C++ compiler (e.g., g++)
- `filesystem` and `fstream` libraries
- `zlib` library for compression support

### **Installation** 📥
1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/ArchiveProject.git

## **How It Works** 🔍

### **Creating an Archive** 🗃️
Use the `createArchive` method to create a new archive. It generates a file with the `.arc` extension, where files will be stored.

### **Adding Files** ➕
Files can be added using the `add` method. Optionally, you can apply a processor (e.g., compression) to the file before storing it in the archive.

### **Extracting Files** 📤
Files are extracted with the `extract` method. The file is retrieved from the archive and saved to the specified directory.

### **Removing Files** 🗑️
Files are removed from the archive using the `remove` method. This method marks the file as "removed" and updates the archive.

### **Listing Files** 📜
You can list all the files stored in the archive using the `list` method. This will display metadata like the file size, name, and date added.

### **Compacting Archives** ⚡
The `compact` method is used to remove empty blocks and shrink the archive to improve storage efficiency.

---

## **Key Concepts** 🧠

### **Chunking**:
The archive is divided into 1024-byte chunks to store files in blocks. Metadata for each file is stored in the header of the chunk.

### **Compression**:
Files are optionally compressed using the `Compression` class. The `process` method compresses files, and the `reverseProcess` method decompresses them when extracted.

### **Metadata**:
Each chunk contains metadata such as:
- `name`: The name of the file.
- `filesize`: The size of the file.
- `comp_size`: The compressed size (if applicable).
- `dateAdded`: The timestamp when the file was added to the archive.
- `checkSum`: A checksum to validate the integrity of the chunk.

---

## **Functions** 📚
- `createArchive()`: Creates a new archive file.
- `openArchive()`: Opens an existing archive file.
- `add()`: Adds a file to the archive.
- `extract()`: Extracts a file from the archive.
- `remove()`: Removes a file from the archive.
- `list()`: Lists all files in the archive.
- `compact()`: Removes empty blocks and shrinks the archive.

---

## **Usage Example** 💻

```cpp
// Example of creating an archive, adding files, and extracting them
auto archive = Archive::createArchive("my_archive.arc");
archive.add("file1.txt", nullptr);
archive.add("file2.txt", nullptr);
archive.extract("file1.txt", "/path/to/extracted/file1.txt");
```

## **License 📝**
This project is licensed under the MIT License. See the LICENSE file for details.

