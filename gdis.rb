class Gdis < Formula
  desc "GDIS — Graphical Display and Interactive Structure viewer (Qt6 port)"
  homepage "https://github.com/ovhpa/gdis"
  # url "https://github.com/ovhpa/gdis/archive/refs/tags/v1.26.tar.gz"
  # sha256 "..."

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build

  depends_on "qt@6"
  depends_on "glib"
  depends_on "cairo"
  depends_on "kdsoap-qt6"

  def install
    system "cmake", "-S", ".", "-B", "build",
      "-DCMAKE_INSTALL_PREFIX=#{prefix}",
      "-DCMAKE_BUILD_TYPE=Release",
      "-DUSE_CAIRO=ON",
      "-DUSE_GRISU=OFF"
    system "cmake", "--build", "build", "-j#{ENV.make_jobs}"
    system "cmake", "--install", "build"
  end

  post_install do
    # Ensure data files are discoverable after installation.
    # The binary resolves its own path at runtime via g_find_program_in_path(argv[0]),
    # so data files placed alongside the binary in bin/ will be found automatically.
    # No additional symlinks or copies needed for standard Homebrew layout.
  end

  test do
    # Verify the binary was built and can report its version/help
    system "#{bin}/gdis", "--help"
  end
end
