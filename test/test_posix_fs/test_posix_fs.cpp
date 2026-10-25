/*
 * Tests for the POSIX and stdio filesystem adapters against a real
 * temporary directory.
 */

#include <unity.h>
#include <microStore/Adapters/PosixFileSystem.h>
#include <microStore/Adapters/StdioFileSystem.h>
#include <microStore/FileStore.h>

#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static std::string tmp;

static bool is_dir(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

/* ---- rmdir ---- */

static void check_rmdir(microStore::FileSystem fs) {
    const std::string dir = tmp + "/sub";
    TEST_ASSERT_TRUE(fs.mkdir(dir.c_str()));
    TEST_ASSERT_TRUE(fs.rmdir(dir.c_str()));
    TEST_ASSERT_FALSE(is_dir(dir));
    TEST_ASSERT_FALSE(fs.rmdir(dir.c_str()));
}

void test_posix_rmdir_reports_result() {
    check_rmdir(microStore::Adapters::PosixFileSystem());
}

void test_stdio_rmdir_reports_result() {
    check_rmdir(microStore::Adapters::StdioFileSystem());
}

/* ---- Base path ---- */

static void write_file(microStore::FileSystem& fs, const char* path) {
    microStore::File f = fs.open(path, microStore::File::ModeWrite, true);
    TEST_ASSERT_TRUE((bool)f);
    TEST_ASSERT_EQUAL(4, f.write("test", 4));
    f.close();
}

void test_posix_relative_paths_resolve_against_basepath() {
    microStore::Adapters::PosixFileSystem fs(tmp.c_str());
    TEST_ASSERT_TRUE(fs.mkdir("./store/"));
    TEST_ASSERT_TRUE(is_dir(tmp + "/store"));
    TEST_ASSERT_TRUE(fs.isDirectory("store"));

    write_file(fs, "./store/a");
    TEST_ASSERT_EQUAL(0, ::access((tmp + "/store/a").c_str(), F_OK));
    TEST_ASSERT_TRUE(fs.exists("store/a"));
    TEST_ASSERT_EQUAL(4, fs.size("./store/a"));
    TEST_ASSERT_EQUAL(1, fs.listDirectory("./store/").size());

    TEST_ASSERT_TRUE(fs.rename("./store/a", "./store/b"));
    TEST_ASSERT_EQUAL(0, ::access((tmp + "/store/b").c_str(), F_OK));
    TEST_ASSERT_TRUE(fs.remove("./store/b"));
    TEST_ASSERT_FALSE(fs.exists("./store/b"));
    TEST_ASSERT_TRUE(fs.rmdir("./store/"));
    TEST_ASSERT_FALSE(is_dir(tmp + "/store"));
}

void test_posix_absolute_paths_ignore_basepath() {
    microStore::Adapters::PosixFileSystem fs("/nonexistent-base");
    const std::string file = tmp + "/abs";
    write_file(fs, file.c_str());
    TEST_ASSERT_TRUE(fs.exists(file.c_str()));
}

void test_posix_basepath_is_copied() {
    std::string* base = new std::string(tmp);
    microStore::Adapters::PosixFileSystem fs(base->c_str());
    base->assign(base->size(), 'x');
    delete base;
    write_file(fs, "./copied");
    TEST_ASSERT_EQUAL(0, ::access((tmp + "/copied").c_str(), F_OK));
}

void test_posix_store_lives_under_basepath() {
    microStore::Adapters::PosixFileSystem fs(tmp.c_str());
    microStore::FileStore store;
    TEST_ASSERT_TRUE(store.init(fs, "./kv/"));
    TEST_ASSERT_TRUE(is_dir(tmp + "/kv"));
}

/* ---- Main ---- */

void setUp() {
    char tmpl[] = "/tmp/microstore_fs_XXXXXX";
    TEST_ASSERT_NOT_NULL(::mkdtemp(tmpl));
    tmp = tmpl;
}

void tearDown() {
    std::string cmd = "rm -rf '" + tmp + "'";
    (void)std::system(cmd.c_str());
}

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_posix_rmdir_reports_result);
    RUN_TEST(test_stdio_rmdir_reports_result);
    RUN_TEST(test_posix_relative_paths_resolve_against_basepath);
    RUN_TEST(test_posix_absolute_paths_ignore_basepath);
    RUN_TEST(test_posix_basepath_is_copied);
    RUN_TEST(test_posix_store_lives_under_basepath);
    return UNITY_END();
}

// For native dev-platform or for some embedded frameworks
int main(void) {
	return runUnityTests();
}

#ifdef ARDUINO
// For Arduino framework
void setup() {
	// Wait ~2 seconds before the Unity test runner
	// establishes connection with a board Serial interface
	delay(2000);

	runUnityTests();
}
void loop() {}
#endif
