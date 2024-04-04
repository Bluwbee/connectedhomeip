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
                
                sh '''#!/bin/bash
                    source scripts/tools/zap/zap_bootstrap.sh
                '''

                sh '''#!/bin/bash
                    export PATH="${PATH}:/home/jenkins/.local/bin"
                    source scripts/activate.sh
                    cd examples/air-quality-sensor-app/linux
                    gn gen out/debug
                    ninja -C out/debug
                '''
            }
        }
    }
}