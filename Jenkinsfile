#!/usr/bin/env groovy
properties([
    gitLabConnection(gitLabConnection: 'EEMH', jobCredentialId: ''),
    parameters([
        booleanParam(name: 'SKLK_NIGHTLY_BUILD', defaultValue: false),
    ]),
])

def gitClean() {
    sh "git clean -ffdx ${WORKSPACE}"
}

def gitlabNotify(String stage, String state) {
    updateGitlabCommitStatus name: stage, state: state
}

def slackresponse = slackSend channel: "#dev-ci-test", botUser: true, color: "warning", message: "Build Started: sklk_phy branch: ${BRANCH_NAME} Build#: 2023.02.01.${BUILD_NUMBER}"
pipeline {
    agent {
        docker {
            alwaysPull true
            reuseNode true
            image "artifactory.ad.sklk.us/gitlabdocker/skylark-wireless/software/sklk-phy-mod:jenkins"
            args '--privileged'
        }
    }
    environment {
        GIT_SUBMODULE_STRATEGY = "recursive"
        GIT_DEPTH = "0"
    }
    // tell the pipeline to use jfrog plugin
    tools {
        jfrog 'Arti'
    }
    stages {
        stage('build-manual') {
            steps {
                gitClean()
                sh 'sudo apt -y update'
                sh 'sudo apt install -y -qq sklk-mii sklk-json sklk-phy'
                dir('build') {
                    sh 'cmake -D SKLK_PHY_MOD_ENABLE_DOCS=1 ..'
                    sh 'make sklk-phy-mod-docs'
                    dir('docs/doxygen') {
                        sh 'make -C latex pdf'
                        sh 'cp latex/refman.pdf ' + env.WORKSPACE + '/sklk-phy-modding-manual.pdf'
                        sh 'tar -vczf  sklk-phy-modding-manual-html.tar.gz html'
                        sh 'cp sklk-phy-modding-manual-html.tar.gz ' + env.WORKSPACE + '/'
                    }
                }
            }
        }
        stage('build-sklk-phy-mod') {
            steps {
                gitClean()
                sshagent(credentials: ['JenkinsGitlabSSH']) {
                    sh "git fetch --prune --prune-tags --tags"
                }

                // Bump nightly version
                sh "./gradlew version"
                script {
                    if (params.SKLK_NIGHTLY_BUILD) {
                        // Only bump version if current commit is not already tagged with a version
                        tagDist = sh(script: 'git rev-list $(git describe --abbrev=0 --tags)..HEAD --count', returnStdout: true).trim()
                        if (tagDist != "0") {
                            sh "./gradlew bumpVersion"
                        }
                        sshagent(credentials: ['JenkinsGitlabSSH']) {
                            sh "git push --tags"
                        }
                    }
                }

                // Build debian
                sh "./gradlew deb-sklk-phy-mod"
                dir('build/sklk-phy-mod/deb') {
                    sh "rm -f ${WORKSPACE}/sklk-phy-mod*.deb"
                    sh "cp -v sklk-phy-mod*.deb ${WORKSPACE}/"
                }
                //Test package install and python imports
                //Uninstall sklk_json to make sure this brings it in
                sh 'sudo apt-get remove -y sklk-json'
                sh 'sudo apt-get install -y ./sklk-phy-mod*.deb'
                dir('build/sklk-phy-mod/cmake-build') {
                    //Run tests
                    sh 'CTEST_OUTPUT_ON_FAILURE=1 RUN_EXTENDED_TESTS=1 ctest'
                }
            }
        }
        stage('export-sklk-phy-mod') {
            steps {
                sh 'git archive --format "tar.gz" --output "sklk_phy_mod.tar.gz"  `git rev-parse --abbrev-ref HEAD`'
            }
        }
        stage('test-export-build') {
            steps {
                sh 'ls *'
                sh 'sudo apt update'
                sh 'sudo apt install -y -qq sklk-mii sklk-json sklk-phy'
                sh "tar -xzvf ${WORKSPACE}/sklk_phy_mod.tar.gz"
                sh 'cmake -S . -B build'
                sh 'make -C build -j package'
            }
        }
        stage('deploy to artifactory') {
            steps {
                script {
                    GIT_SHORT_HASH = sh(script: 'git rev-parse --short HEAD', returnStdout: true).trim()
                    COMMIT_COUNT = sh(script: 'git rev-list --count HEAD', returnStdout: true).trim()

                    VERSION = sh(script: "./gradlew version --quiet", returnStdout: true).trim()
                    TARGET_PROPS = "version=${VERSION};build_node=${NODE_NAME};pipeline=${BUILD_NUMBER};branch=${BRANCH_NAME};hash=${GIT_COMMIT},hash_short=${GIT_SHORT_HASH}"
                }

                jf "rt u ${WORKSPACE}/(*.deb) jenkins-sklk-deb/pool/sklk_phy_mod/{1} --flat --detailed-summary --recursive=false --project=jenkci --build-name=sklk_phy_mod --build-number=2023.02.01.${BUILD_NUMBER} --deb focal/testing/amd64 --target-props=${TARGET_PROPS}"
                jf "rt u ${WORKSPACE}/(*.pdf) jenkins-sklk-artifacts/sklk_phy_mod/pipelines/${BRANCH_NAME}/${BUILD_NUMBER}/{1} --flat --detailed-summary --recursive=false --project=jenkci --build-name=sklk_phy_mod --build-number=2023.02.01.${BUILD_NUMBER} --target-props=${TARGET_PROPS}"
                jf "rt u ${WORKSPACE}/(*.tar.gz) jenkins-sklk-artifacts/sklk_phy_mod/pipelines/${BRANCH_NAME}/${BUILD_NUMBER}/{1} --flat --detailed-summary --recursive=false --project=jenkci --build-name=sklk_phy_mod --build-number=2023.02.01.${BUILD_NUMBER} --target-props=${TARGET_PROPS}"
                jf "rt u ${WORKSPACE}/(*.pdf) jenkins-sklk-artifacts/sklk_phy_mod/latest/${BRANCH_NAME}/{1} --flat --detailed-summary --recursive=false --project=jenkci --build-name=sklk_phy_mod --build-number=2023.02.01.${BUILD_NUMBER} --target-props=${TARGET_PROPS}"
                jf "rt u ${WORKSPACE}/(*.tar.gz) jenkins-sklk-artifacts/sklk_phy_mod/latest/${BRANCH_NAME}/{1} --flat --detailed-summary --recursive=false --project=jenkci --build-name=sklk_phy_mod --build-number=2023.02.01.${BUILD_NUMBER} --target-props=${TARGET_PROPS}"
                //upload build info
                // clean first
                jf 'rt bc'
                //collect build env
                jf "rt bce --project=jenkci sklk_phy_mod ${BUILD_NUMBER}"
                //add the git info
                jf "rt bag --project=jenkci sklk_phy_mod ${BUILD_NUMBER}"
                //publish the build info
                jf "rt bp --project=jenkci sklk_phy_mod ${BUILD_NUMBER}"
            }

        }

    }
    post {
        failure {
            slackSend channel: slackresponse.channelId, color: "danger", botUser: true, message: "Build Failed: sklk_phy_mod branch: ${BRANCH_NAME} on node ${NODE_NAME} build#: ${VERSION}", timestamp: slackresponse.ts
        }
        success {
            slackSend channel: slackresponse.channelId, color: "good", botUser: true, message: "Build Complete: sklk_phy_mod branch: ${BRANCH_NAME} on node ${NODE_NAME}  build#: ${VERSION}", timestamp: slackresponse.ts
        }
    }
}
