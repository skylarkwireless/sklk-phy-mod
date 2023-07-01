
plugins {
    id("com.skylarkwireless.gradle.builds") version "latest.release"
}

builds {
    cmake("sklk-phy-mod", layout.projectDirectory) {
        deb {
            requiresExact("sklk-phy")
            requiresExact("sklk-json")
            requiresExact("sklk-mii")

            conflicts("sklk_tools")
        }
    }
}
