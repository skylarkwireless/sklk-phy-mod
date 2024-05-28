deriveParams()
def slackResponse = sklkSlack.notifyStart()

pipeline {
    agent none
    options {
        skipDefaultCheckout true
        gitLabConnection('EEMH')
        disableConcurrentBuilds(abortPrevious: true)
    }
    // tell the pipeline to use jfrog plugin
    tools {
        jfrog 'Arti'
    }

    stages {
        stage('versioning') {
            steps {
                // Perform checks
                runDocker {
                    sshagent(credentials: ['JenkinsGitlabSSH']) {
                        sh 'git fetch --prune --prune-tags --tags'
                    }
                    sh './gradlew check'

                    sh './gradlew version'

                    // If on master, bump version
                    script {
                        if (env.BRANCH_NAME == 'master') {
                            sh './gradlew bumpVersion'

                            sshagent(credentials: ['JenkinsGitlabSSH']) {
                                sh 'git push --tags'
                            }
                        }
                    }

                    // Check version
                    sh './gradlew version'
                }
            }
        }
        stage('build') {
            failFast true
            parallel {
                stage('build-release') {
                    steps {
                        runDocker(true, env.STAGE_NAME) {
                            sh 'sudo apt -y update'
                            sh 'sudo apt install -y sklk-phy sklk-cpptest'
                            dir('build') {
                                sh 'cmake -DSKLK_FORCE_ASSERTS=OFF ..'
                                sh 'make -j'
                                sh 'ctest --output-on-failure --schedule-random'
                            }
                        }
                    }
                }
                stage('build-debug') {
                    steps {
                        runDocker(true, env.STAGE_NAME) {
                            sh 'sudo apt -y update'
                            sh 'sudo apt install -y sklk-phy sklk-cpptest'
                            dir('build') {
                                sh 'cmake -DCMAKE_BUILD_TYPE=Debug ..'
                                sh 'make -j'
                                sh 'ctest --output-on-failure --schedule-random'
                            }
                        }
                    }
                }
                stage('sklk-phy-mod') {
                    steps {
                        runDocker(true, env.STAGE_NAME) {
                            sh 'sudo apt update'
                            sh 'sudo apt install -y sklk-phy'

                            // Build debian
                            sh './gradlew deb-sklk-phy-mod'
                            dir('build/sklk-phy-mod/deb') {
                                sh 'sudo apt-get install -y ./sklk-phy-mod*.deb'
                                uploadDeb('sklk-phy-mod')
                            }
                            publishBi(env.BUILDVER)
                        }
                    }
                }
            }
        }
        // stage('push build details to jfrog') {
        //     steps {
        //         gitlabNotify()
        //         publishBi()
        //     }
        //     post {
        //         failure {
        //             gitlabNotify('failed')
        //         }
        //         success {
        //             gitlabNotify('success')
        //         }
        //         aborted {
        //             gitlabNotify('canceled')
        //         }
        //         cleanup {
        //             gitClean()
        //         }
        //     }
        // }
    }
    post {
        always {
            script {
                sklkSlack.notifyComplete(slackResponse)
            }
        }
        failure {
            gitlabNotify('failed')
        }
        success {
            gitlabNotify('success')
        }
        aborted {
            gitlabNotify('canceled')
        }
    }
}

