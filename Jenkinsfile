pipeline
{
    agent { dockerfile true }
    
    stages
    {
        stage("Building")
        {
            environment{
                DEBIAN_FRONTEND = 'noninteractive'
                ZAP_DEVELOPMENT_PATH = "${WORKSPACE}/zap-dir"
                CYPRESS_CACHE_FOLDER = "${WORKSPACE}/.cypress/.cache"
                N_PREFIX = "${WORKSPACE}/.n"
                ARGS_VAR = "ti_sysconfig_root=\"$HOME/ti/sysconfig_1.13.0\"" 
            }
            steps
            {
                sh 'sudo npm install n -g'
                sh 'sudo n 19.0.0'
                sh 'node --version'

                sh 'git clean -xdf'
                
                // Download and install ZAP
                dir('zap-dir'){
                    checkout scmGit(branches: [[name: 'refs/tags/v2023.10.30-nightly']], extensions: [], userRemoteConfigs: [[credentialsId: '0290ee72-6c30-41a5-9d3a-ff55551eb0b8', url: 'https://github.com/project-chip/zap.git']])
                }
                
                // Download and install syscfg
                dir('get-sysconfig'){
                    sh '''#!/bin/bash
                        wget https://software-dl.ti.com/ccs/esd/sysconfig/sysconfig-1.13.0_2553-setup.run
                        chmod +x sysconfig-1.13.0_2553-setup.run
                    '''
                    sh './sysconfig-1.13.0_2553-setup.run --mode unattended'
                }
                
                sh '''#!/bin/bash
                    source scripts/tools/zap/zap_bootstrap.sh
                '''

                sh '''#!/bin/bash
                    export PATH="${PATH}:/home/jenkins/.local/bin"
                    source scripts/activate.sh
                    cd examples/lock-app/cc32xx
                    gn gen out/debug --args=${ARGS_VAR}
                    ninja -C out/debug
                '''
            }
        }
    }
}